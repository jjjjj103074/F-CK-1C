import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import {execFileSync} from 'node:child_process';
const root=process.cwd(), report=path.join(process.env.RUNNER_TEMP,'efm-submission','repro-diagnostics.json');
const hash=b=>crypto.createHash('sha256').update(b).digest('hex').toUpperCase();
function describe(b){
 const pe=b.readUInt32LE(0x3c), count=b.readUInt16LE(pe+6), optional=b.readUInt16LE(pe+20), table=pe+24+optional;
 const sections=[];
 for(let i=0;i<count;i++){const p=table+i*40,size=b.readUInt32LE(p+16),start=b.readUInt32LE(p+20);sections.push({name:b.subarray(p,p+8).toString('ascii').replaceAll('\0',''),sha256:hash(b.subarray(start,start+size))});}
 const rsds=b.indexOf(Buffer.from('RSDS'));
 return{size:b.length,sha256:hash(b),sections,debugSignature:rsds<0?null:b.subarray(rsds+4,rsds+20).toString('hex')};
}
const uploaded=fs.readFileSync(path.join(root,'bin/F-CK-1C_EFM.dll'));
const rebuilt=fs.readFileSync(path.join(root,'src/efm/x64/Release/F-CK-1C_EFM.dll'));
const a=describe(uploaded),b=describe(rebuilt);
let differingBytes=0;for(let i=0;i<Math.max(uploaded.length,rebuilt.length);i++)if(uploaded[i]!==rebuilt[i])differingBytes++;
const data={uploadedSha256:a.sha256,rebuiltSha256:b.sha256,sameSize:a.size===b.size,differingBytes,debugSignatureMatches:a.debugSignature===b.debugSignature,sections:b.sections.map(s=>({name:s.name,matches:a.sections.find(t=>t.name===s.name)?.sha256===s.sha256}))};
// Hash only the public GitHub runner's standard installed inputs.
// No fingerprints or other data from the developer computer are sent here.
const p86=process.env['ProgramFiles(x86)'];
const instances=JSON.parse(execFileSync(path.join(p86,'Microsoft Visual Studio/Installer/vswhere.exe'),['-all','-products','*','-requires','Microsoft.Component.MSBuild','-format','json'],{encoding:'utf8'}));
const tools=instances.map(v=>path.join(v.installationPath,'VC/Tools/MSVC/14.29.30133')).find(p=>fs.existsSync(path.join(p,'bin/Hostx64/x64/cl.exe'))),sdk=path.join(p86,'Windows Kits/10');
function aggregate(dir){const entries=[];function walk(current,prefix){for(const e of fs.readdirSync(current,{withFileTypes:true})){const f=path.join(current,e.name),relative=prefix+e.name;if(e.isDirectory())walk(f,relative+'/');else if(e.isFile()&&(/\.(h|hpp|hxx|inc|inl|lib|obj)$/i.test(e.name)))entries.push([relative.toLowerCase(),crypto.createHash('sha256').update(fs.readFileSync(f)).digest('hex')]);}}walk(dir,'');entries.sort((a,b)=>a[0]<b[0]?-1:a[0]>b[0]?1:0);return crypto.createHash('sha256').update(entries.map(([name,hash])=>name+'='+hash).join('\n')).digest('hex');}
data.runnerInputDigests={msvcHeaders:aggregate(path.join(tools,'include')),msvcLibraries:aggregate(path.join(tools,'lib/x64')),sdkHeaders:aggregate(path.join(sdk,'Include/10.0.26100.0')),ucrtLibraries:aggregate(path.join(sdk,'Lib/10.0.26100.0/ucrt/x64')),umLibraries:aggregate(path.join(sdk,'Lib/10.0.26100.0/um/x64'))};
fs.mkdirSync(path.dirname(report),{recursive:true});fs.writeFileSync(report,JSON.stringify(data,null,2));console.log(JSON.stringify(data));
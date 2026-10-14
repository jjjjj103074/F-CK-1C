import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
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
fs.mkdirSync(path.dirname(report),{recursive:true});fs.writeFileSync(report,JSON.stringify(data,null,2));console.log(JSON.stringify(data));
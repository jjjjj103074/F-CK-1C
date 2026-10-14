// Diagnostic only: inspect Microsoft's standard libraries on the public runner.
// These partial digests never replace the production whole-DLL hash check.
import crypto from 'node:crypto';
const sha=bytes=>crypto.createHash('sha256').update(bytes).digest('hex');
export function librarySections(bytes) {
  if(bytes.subarray(0,8).toString()!=='!<arch>\n')throw new Error('Not a COFF library');
  let position=8,longNames=Buffer.alloc(0),objects=0;
  const code=[],data=[];
  while(position+60<=bytes.length){
    const header=bytes.subarray(position,position+60),size=Number(header.subarray(48,58).toString().trim());
    let name=header.subarray(0,16).toString().trim();
    const object=bytes.subarray(position+60,position+60+size);
    position+=60+size+(size%2);
    if(name==='//'){longNames=object;continue;}
    if(name==='/')continue;
    if(/^\/\d+$/.test(name)){const offset=Number(name.slice(1));let end=longNames.indexOf(0,offset);if(end<0)end=longNames.length;name=longNames.subarray(offset,end).toString().trim();}
    name=name.replaceAll('\\','/').split('/').at(-1);
    const big=object.length>=56&&object.readUInt16LE(0)===0&&object.readUInt16LE(2)===65535&&object.readUInt16LE(4)>=2;
    const count=big?object.readUInt32LE(44):object.length>=20?object.readUInt16LE(2):0;
    const start=big?56:object.length>=20?20+object.readUInt16LE(16):0;
    if(!count||count>10000||start+count*40>object.length)continue;
    objects++;
    for(let i=0;i<count;i++){
      const section=start+i*40,sectionName=object.subarray(section,section+8).toString().replaceAll('\0','');
      const length=object.readUInt32LE(section+16),offset=object.readUInt32LE(section+20);
      if(!length||offset+length>object.length)continue;
      const entry=name+'|'+sectionName+'|'+sha(object.subarray(offset,offset+length));
      if(sectionName.startsWith('.text'))code.push(entry);
      if(sectionName.startsWith('.rdata')||sectionName.startsWith('.data'))data.push(entry);
    }
  }
  return {objects,codeSections:code.length,dataSections:data.length,codeDigest:sha(code.sort().join('\n')),dataDigest:sha(data.sort().join('\n'))};
}

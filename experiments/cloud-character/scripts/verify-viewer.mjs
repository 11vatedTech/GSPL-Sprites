import { chromium } from 'playwright';
import assert from 'node:assert/strict';
import { writeFile } from 'node:fs/promises';
const browser=await chromium.launch({headless:true,args:['--use-angle=swiftshader','--enable-unsafe-swiftshader']});
try{
 const page=await browser.newPage({viewport:{width:1440,height:1100}});
 const errors=[];page.on('pageerror',e=>errors.push(e.message));page.on('console',m=>{if(m.type()==='error')errors.push(m.text())});
 await page.goto(process.env.VIEWER_URL||'http://127.0.0.1:4173/',{waitUntil:'networkidle',timeout:120000});
 await page.waitForFunction(()=>window.__assetStatus?.ready===true,{timeout:120000});
 const r=await page.evaluate(()=>window.__assetStatus);
 assert.ok(r.triangles>=30000,'High-detail mesh threshold failed: '+r.triangles);
 assert.ok(r.bones>=20,'Skeleton threshold failed: '+r.bones);
 assert.ok(r.skinnedMeshes>=1,'No skinned mesh');
 assert.ok(r.clips>=10,'Animation threshold failed: '+r.clips);
 assert.ok(r.hasIdle&&r.hasWalk&&r.hasRun&&r.hasAttack,'Idle/locomotion/combat clips missing');
 const walkName=r.clipNames.find(n=>/walk/i.test(n));assert.ok(walkName,'Walk animation name missing');await page.locator('#clips').selectOption(walkName);const poseBefore=await page.evaluate(()=>window.__poseProbe());await page.waitForTimeout(850);const poseAfter=await page.evaluate(()=>window.__poseProbe());assert.ok(poseBefore.some((v,i)=>Math.abs(v-poseAfter[i])>0.0001),'Animation playback did not deform the skeleton');const before=await page.evaluate(()=>window.__movementProbe);await page.keyboard.down('w');await page.waitForTimeout(900);await page.keyboard.up('w');
 const after=await page.evaluate(()=>window.__movementProbe);
 assert.ok(Math.hypot(after.x-before.x,after.z-before.z)>.2,'W input did not move character root');
 await page.screenshot({path:'cloud-character-preview.png',fullPage:true});
 assert.equal(errors.length,0,errors.join('\n'));
 const report={passed:true,...r,movementDistance:Number(Math.hypot(after.x-before.x,after.z-before.z).toFixed(3)),consoleErrors:errors,screenshot:'cloud-character-preview.png'};
 await writeFile('cloud-character-verification.json',JSON.stringify(report,null,2)+'\n');console.log(JSON.stringify(report,null,2));
}finally{await browser.close()}

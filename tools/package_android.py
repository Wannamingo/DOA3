from pathlib import Path
import argparse,subprocess,zipfile,hashlib,json,shutil,re,os
parser=argparse.ArgumentParser()
parser.add_argument('--toolchains',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
root=Path(__file__).resolve().parents[1];tools=args.toolchains.resolve();out=args.output.resolve()
out.mkdir(parents=True,exist_ok=True)
build=root/'private/apk';build.mkdir(parents=True,exist_ok=True)
native=root/'private/build-arm64/libmain.so'
receipt=json.loads((root/'private/build-arm64/build-receipt.json').read_text())
recipe=root/'tools/game-recipe/usa-android.json';identity=json.loads(recipe.read_text())
assert receipt['recipe_sha256']==hashlib.sha256(recipe.read_bytes()).hexdigest()
assert receipt['native_sha256']==hashlib.sha256(native.read_bytes()).hexdigest()
for key in ['xbe_sha256','program_manifest_sha256']:assert receipt[key]==identity[key]
jdk=tools/'jdk-21.0.12.1+1/bin';sdk=tools/'sdk/android-15';android=tools/'sdk/android-35/android.jar';sdl=tools/'SDL3-android/aar'
llvm=tools/'android-ndk-r30/toolchains/llvm/prebuilt/windows-x86_64/bin'
env=dict(os.environ,TMP=str(build),TEMP=str(build))
def run(cmd):
 result=subprocess.run(list(map(str,cmd)),capture_output=True,text=True,encoding='utf-8',errors='replace',env=env)
 if result.returncode:raise RuntimeError(result.stdout+result.stderr)
 return result.stdout+result.stderr
symbols=run([llvm/'llvm-nm.exe','-D',native])
java=root/'android/java'
for p in java.rglob('*.java'):
 text=p.read_text(encoding='utf-8');package=re.search(r'package\s+([\w.]+);',text).group(1)
 for method in re.findall(r'\bnative\s+\w+\s+(\w+)\s*\(',text):
  symbol='Java_'+package.replace('.','_')+'_'+p.stem+'_'+method
  assert re.search(r'\bT '+re.escape(symbol)+r'\s*$',symbols,re.M),symbol
classes=build/'classes';dex=build/'dex';classes.mkdir(exist_ok=True);dex.mkdir(exist_ok=True)
run([jdk/'javac.exe','--release','17','-encoding','UTF-8','-classpath',str(android)+';'+str(sdl/'classes.jar'),'-d',classes,*sorted(java.rglob('*.java'))])
jar=build/'app-classes.jar';run([jdk/'jar.exe','--create','--file',jar,'-C',classes,'.'])
run([jdk/'java.exe','-Djava.io.tmpdir='+str(build),'-cp',sdk/'lib/d8.jar','com.android.tools.r8.D8','--min-api','26','--lib',android,'--output',dex,jar,sdl/'classes.jar'])
resources=build/'resources.zip';run([sdk/'aapt2.exe','compile','--dir',root/'android/res','-o',resources])
unsigned=build/'unsigned.apk';run([sdk/'aapt2.exe','link','-o',unsigned,'--manifest',root/'android/AndroidManifest.xml','-I',android,'--min-sdk-version','26','--target-sdk-version','35',resources])
library=build/'libmain.so';shutil.copy2(native,library);run([llvm/'llvm-strip.exe','--strip-debug',library])
with zipfile.ZipFile(unsigned,'a') as archive:
 archive.write(root/'private/disc-identity.json','assets/disc-identity.json',compress_type=zipfile.ZIP_DEFLATED)
 for p in (root/'android/assets').rglob('*'):
  if p.is_file():archive.write(p,'assets/'+p.relative_to(root/'android/assets').as_posix(),compress_type=zipfile.ZIP_DEFLATED)
 for p in (root/'android/third_party/notices').glob('*'):
  if p.is_file():archive.write(p,'assets/third-party/'+p.name,compress_type=zipfile.ZIP_DEFLATED)
 archive.write(root/'LICENSE','assets/third-party/runtime-GPL-3.0.txt',compress_type=zipfile.ZIP_DEFLATED)
 archive.write(root/'android/third_party/xxhash/LICENSE','assets/third-party/xxhash-LICENSE.txt',compress_type=zipfile.ZIP_DEFLATED)
 for p in dex.glob('classes*.dex'):archive.write(p,p.name,compress_type=zipfile.ZIP_DEFLATED)
 archive.write(library,'lib/arm64-v8a/libmain.so',compress_type=zipfile.ZIP_STORED)
 archive.write(sdl/'prefab/modules/SDL3-shared/libs/android.arm64-v8a/libSDL3.so','lib/arm64-v8a/libSDL3.so',compress_type=zipfile.ZIP_STORED)
aligned=build/'aligned.apk';run([sdk/'zipalign.exe','-P','16','-f','4',unsigned,aligned])
key=build/'local-debug.jks'
if not key.exists():run([jdk/'keytool.exe','-genkeypair','-keystore',key,'-storepass','android','-keypass','android','-alias','androiddebugkey','-keyalg','RSA','-keysize','2048','-validity','3650','-dname','CN=Local DOA3 Android Development'])
apk=out/'DOA3-RG477M-arm64-alpha1.apk'
run([jdk/'java.exe','-jar',sdk/'lib/apksigner.jar','sign','--ks',key,'--ks-key-alias','androiddebugkey','--ks-pass','pass:android','--key-pass','pass:android','--out',apk,aligned])
verification=run([jdk/'java.exe','-jar',sdk/'lib/apksigner.jar','verify','--verbose',apk])
run([sdk/'zipalign.exe','-c','-P','16','4',apk])
badging=run([sdk/'aapt2.exe','dump','badging',apk])
assert "package: name='org.doa3.port'" in badging and "launchable-activity: name='org.doa3.port.BeachLauncherActivity'" in badging
with zipfile.ZipFile(apk) as archive:
 assert archive.testzip() is None
 assert archive.read('assets/launcher/background.jpg')==(root/'android/assets/launcher/background.jpg').read_bytes()
(build/'signature-verification.txt').write_text(verification)
print(json.dumps({'apk':apk.name,'bytes':apk.stat().st_size,'sha256':hashlib.sha256(apk.read_bytes()).hexdigest(),'abi':'arm64-v8a','aligned_16k':True},indent=2))

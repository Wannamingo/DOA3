"""Build the authenticated DOA3 ARM64 native library with a local toolchain."""
from pathlib import Path
import argparse, subprocess, hashlib, json, os

parser=argparse.ArgumentParser()
parser.add_argument('--toolchains',type=Path,required=True)
parser.add_argument('--jobs',type=int,default=2)
parser.add_argument('--skip-configure',action='store_true')
parser.add_argument('targets',nargs='*',default=['main'])
args=parser.parse_args()
root=Path(__file__).resolve().parents[1]
tools=args.toolchains.resolve()
build=root/'private/build-arm64'
build.mkdir(parents=True,exist_ok=True)
temporary=build/'tmp'
temporary.mkdir(exist_ok=True)
environment=dict(os.environ,TEMP=str(temporary),TMP=str(temporary))
recipe=root/'tools/game-recipe/usa-android.json'
cmake=tools/'cmake/bin/cmake.exe'
command=[cmake,'-S',root/'android','-B',build,'-G','Ninja',
 '-DCMAKE_MAKE_PROGRAM='+str(tools/'ninja/ninja.exe'),
 '-DCMAKE_TOOLCHAIN_FILE='+str(tools/'android-ndk-r30/build/cmake/android.toolchain.cmake'),
 '-DANDROID_ABI=arm64-v8a','-DANDROID_PLATFORM=android-26','-DCMAKE_BUILD_TYPE=RelWithDebInfo',
 '-DGAME_DIR='+str(root/'private/game/generated'),
 '-DGAME_RECIPE='+str(recipe),
 '-DGAME_RECIPE_SHA256='+hashlib.sha256(recipe.read_bytes()).hexdigest(),
 '-DGLSLANG_DIR='+str(tools/'glslang-16.6.0'),
 '-DSPIRV_CROSS_DIR='+str(tools/'SPIRV-Cross-aa217aeb6c9f0ace7a0ab233b28807edf45eb165'),
 '-DSDL_ANDROID_ROOT='+str(tools/'SDL3-android/aar')]
steps=[] if args.skip_configure else [('configure',command)]
steps.append(('build',[cmake,'--build',build,'--target',*(args.targets or ['main']),'--parallel',str(args.jobs)]))
for label,cmd in steps:
 log=root/f'private/android-{label}.log'
 print(label,flush=True)
 with log.open('w',encoding='utf-8') as output:
  result=subprocess.run(list(map(str,cmd)),stdout=output,stderr=subprocess.STDOUT,env=environment)
 print('\n'.join(log.read_text(encoding='utf-8',errors='replace').splitlines()[-45:]),flush=True)
 if result.returncode: raise SystemExit(result.returncode)
if 'main' in (args.targets or ['main']):
 identity=json.loads(recipe.read_text())
 (build/'build-receipt.json').write_text(json.dumps({
  'game':'Dead or Alive 3 USA','recipe_sha256':hashlib.sha256(recipe.read_bytes()).hexdigest(),
  'xbe_sha256':identity['xbe_sha256'],'program_manifest_sha256':identity['program_manifest_sha256'],
  'native_sha256':hashlib.sha256((build/'libmain.so').read_bytes()).hexdigest(),
  'status':'compiled; device gameplay verification pending'},indent=2)+'\n')

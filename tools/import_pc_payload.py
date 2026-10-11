"""Import authenticated local inputs from the user's prepared DOA3 PC port."""
from pathlib import Path
import argparse,hashlib,json,shutil
parser=argparse.ArgumentParser();parser.add_argument('pc_port',type=Path);args=parser.parse_args()
root=Path(__file__).resolve().parents[1];source=args.pc_port.resolve()
recipe=json.loads((root/'tools/game-recipe/usa-android.json').read_text())
candidates=list((source/'private').glob('program-*/generated'))
selected=None
for folder in candidates:
 if all((folder/name).is_file() and hashlib.sha256((folder/name).read_bytes()).hexdigest()==digest for name,digest in recipe['generated_files'].items()):
  selected=folder;break
if selected is None:raise SystemExit('No generated DOA3 snapshot matches the pinned Android recipe.')
disc=source/'private/imported-disc/disc'
if hashlib.sha256((disc/'default.xbe').read_bytes()).hexdigest()!=recipe['xbe_sha256']:raise SystemExit('Wrong XBE revision.')
destination=root/'private/game/generated';destination.mkdir(parents=True,exist_ok=True)
for name in recipe['generated_files']:shutil.copy2(selected/name,destination/name)
identity={}
for p in sorted(disc.rglob('*')):
 if not p.is_file():continue
 digest=hashlib.sha256()
 with p.open('rb') as stream:
  while chunk:=stream.read(4*1024*1024):digest.update(chunk)
 identity[p.relative_to(disc).as_posix()]={'size':p.stat().st_size,'sha256':digest.hexdigest()}
(root/'private/disc-identity.json').write_text(json.dumps(identity,indent=2)+'\n')
print('Authenticated generated code and disc identities imported locally.')

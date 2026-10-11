"""Verify a real game ISO using the exact Java reader shipped in the APK."""
from pathlib import Path
import argparse,json,subprocess
p=argparse.ArgumentParser()
p.add_argument('iso',type=Path)
p.add_argument('--toolchains',type=Path,required=True)
a=p.parse_args()
root=Path(__file__).resolve().parents[1]
test=root/'private/iso-test';test.mkdir(parents=True,exist_ok=True)
identity=json.loads((root/'private/disc-identity.json').read_text())
manifest=test/'expected.tsv'
manifest.write_text(''.join(f'{name}\t{file["size"]}\t{file["sha256"]}\n' for name,file in identity.items()),encoding='utf-8')
jdk=a.toolchains/'jdk-21.0.12.1+1/bin'
subprocess.run([str(jdk/'javac.exe'),'-encoding','UTF-8','-d',str(test),str(root/'android/java/org/doa3/port/XboxIso.java'),str(root/'android/tests/org/doa3/port/XboxIsoIntegrationTest.java')],check=True)
subprocess.run([str(jdk/'java.exe'),'-cp',str(test),'org.doa3.port.XboxIsoIntegrationTest',str(a.iso),str(manifest)],check=True)

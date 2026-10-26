# Git 보관본의 원시 바이트를 검증하고 기존 준비본을 덮지 않는 재개 복사만 수행한다.
import argparse,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]  # 이 파일은 저장소 docs/exe/prepared에 둔다.
PREPARED=Path(__file__).resolve().parent  # 원시 patch/base/fixture를 보관한 고정 부모다.
PACKAGES=('new-fort-469-20261027','subscriber-write-469-20261027','data-manager-initialize-469-20261027','add-island-469-20261027','tutorial3-partial-468-20261027')  # 미적용 네 구현 준비본과 미완주 입력 기록이다.

def verified():
    # 모든 파일을 확인하기 전에는 출력 디렉터리를 만들지 않는다.
    packages=[]
    for name in PACKAGES:
        folder=PREPARED/name;manifest=json.loads((folder/'archive-files.json').read_text(encoding='utf-8'));directory=manifest['restoreDirectory']  # 원래 tools의 상대 ROOT 계산을 유지할 복원 이름이다.
        if not directory or Path(directory).name!=directory or directory in ('.','..'): raise RuntimeError('복원 디렉터리 이름 범위 위반')
        for row in manifest['files']:
            path=(folder/row['path']).resolve()  # 보관 root 밖의 절대/상대 경로를 거부한다.
            if not path.is_relative_to(folder.resolve()) or not path.is_file() or path.stat().st_size!=row['bytes'] or hashlib.sha256(path.read_bytes()).hexdigest()!=row['sha256']: raise RuntimeError('보관 바이트 불일치:'+str(path))
        packages.append((folder,manifest))
    return packages

def main():
    # 기본 실행은 읽기 전용 검증이며 --restore를 명시할 때만 새 extracted 준비 디렉터리를 만든다.
    parser=argparse.ArgumentParser(description='469 미적용 준비본의 바이트 검증/새 경로 복원');parser.add_argument('--restore',action='store_true');args=parser.parse_args();packages=verified()
    if args.restore:
        targets=[ROOT/'extracted'/manifest['restoreDirectory'] for _,manifest in packages]  # 원래 분석 도구의 ROOT 계산과 같은 경로다.
        if any(path.exists() for path in targets): raise RuntimeError('기존 준비 디렉터리가 있어 복원을 중단합니다. 기존 파일을 이동/삭제하거나 덮어쓰지 않습니다.')
        for (folder,manifest),target in zip(packages,targets):
            target.mkdir(parents=True)
            for row in manifest['files']:
                path=target/row['path'];path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes((folder/row['path']).read_bytes())  # 어떤 줄바꿈/공백도 변환하지 않는다.
    print(json.dumps(dict(packages=len(packages),files=sum(len(manifest['files']) for _,manifest in packages),bytes=sum(row['bytes'] for _,manifest in packages for row in manifest['files']),restored=args.restore,cppBuildExecuted=False,nativeReplayExecuted=False),ensure_ascii=False))

if __name__=='__main__': main()  # 복원 검증은 제품 채택·C++ 실행 통과와 구별한다.

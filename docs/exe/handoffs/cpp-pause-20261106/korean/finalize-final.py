"""최종 한국어 준비 트리의 패치·해시·보존 증거만 생성한다. 제품이나 게임을 실행하지 않는다."""
from pathlib import Path
import collections,difflib,hashlib,json,re,subprocess

OUT=Path(__file__).resolve().parent # 최종 인계 파일만 이 준비 폴더에 쓴다.
ROOT=OUT.parents[1] # 제품은 해시/내용 대조만 하는 공유 저장소다.
FINAL=OUT/'final-pending' # 두 고정 pending을 병합한 유일한 적용 후보 사본이다.
BASE=OUT/'final-base' # 985 제품의 원래 바이트를 보관한 적용 기준이다.
FROZEN=OUT.parent/'cpp-korean-preparation' # 앞 단계 44개 번역 준비본이며 수정하지 않는다.

# 실제 파일 바이트의 SHA256을 기록해 줄끝/BOM까지 대조한다.
def sha(path):
    """존재하는 파일의 소문자 SHA256을 반환한다."""
    return hashlib.sha256(path.read_bytes()).hexdigest()

# 이전 통합/UI 준비본의 manifest와 고정 사본을 대조한다.
def prior_integrity():
    """기존 두 pending의 저장된 SHA가 현재도 같은지 검사한다."""
    problems=[]
    first=json.loads((OUT/'source-manifest.json').read_text('utf-8-sig'))
    for row in first['files']: # 기존 통합69개는 한 글자도 수정하지 않는다.
        if sha(OUT/'pending'/row['path'])!=row['pendingSha256']:problems.append('integration:'+row['path'])
    ui=OUT.parent/'cpp-korean-ui-preparation'
    second=json.loads((ui/'manifest.json').read_text('utf-8-sig'))
    for row in second['files']: # 입력 담당의 기존16개도 그대로 유지한다.
        if sha(ui/'pending'/row['path'])!=row['new_sha256']:problems.append('ui:'+row['path'])
    return problems

# 최종 코드/스크립트/CMake/검사만 하나의 LF 패치로 생성한다. 자산은 바이트 사본으로 별도 적용한다.
def code_patch():
    """최신 제품 원본 대비 중복 없는 경로별 unified diff를 저장한다."""
    pieces=[];files=[];drift=[]
    for path in sorted((FINAL/'cpppj').rglob('*')): # 원문/fixture/WB/제품 파일을 쓰는 반복이 아니다.
        if not path.is_file():continue
        name=path.relative_to(FINAL).as_posix();before=BASE/name;current=ROOT/name
        old=before.read_bytes().decode('utf-8').replace('\r\n','\n') if before.exists() else ''
        new=path.read_bytes().decode('utf-8').replace('\r\n','\n')
        if before.exists() and (not current.exists() or sha(before)!=sha(current)):drift.append(name)
        if old==new:continue
        pieces.append('diff --git a/'+name+' b/'+name+'\n')
        if not before.exists():pieces.append('new file mode 100644\n')
        pieces.extend(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile='a/'+name if before.exists() else '/dev/null',tofile='b/'+name))
        files.append({'path':name,'baseSha256':sha(before) if before.exists() else None,'currentProductSha256':sha(current) if current.exists() else None,'finalSha256':sha(path),'bytes':path.stat().st_size,'newFile':not before.exists()})
    if drift:raise ValueError('최종 접합 뒤 제품 기준 변경:'+repr(drift))
    (OUT/'final.patch').write_bytes(''.join(pieces).encode('utf-8'))
    return files

# 고정44와 최종44의 차이는 휴면 진단 표시문구 두 개에 한정한다.
def assets():
    """BOM·UTF8·해시와 config 표시문장 외의 원래 바이트 보존을 확인한다."""
    manifest=json.loads((FROZEN/'combined-manifest.json').read_text('utf-8-sig'));rows=[];problems=[];sources=[];expected=[]
    for record in manifest['assets']: # 원래 번역본과 WB 입력은 읽기만 한다.
        original=FROZEN/'merged'/record['path'];name='assets/lang/korean/'+record['path'].removeprefix('lang/');path=FINAL/name;raw=path.read_bytes();before=original.read_bytes()
        if sha(original)!=record['sha256']:problems.append('고정44 변조:'+record['path'])
        if not raw.startswith(b'\xef\xbb\xbf'):problems.append('최종 BOM 없음:'+name)
        text=raw[3:].decode('utf-8');frozen=before[3:].decode('utf-8')
        if record['path']!='d/config.korean' and raw!=before:problems.append('허용 외 자산 차이:'+name)
        if record['path']=='d/config.korean':
            oldkeys=re.findall(r'^([^\s;#\[\]][^=\r\n]*?)=',frozen,re.M);newkeys=re.findall(r'^([^\s;#\[\]][^=\r\n]*?)=',text,re.M)
            if oldkeys!=newkeys:problems.append('config 키/순서 변경')
            delta=list(difflib.ndiff(frozen.splitlines(),text.splitlines()));removed=[v[2:] for v in delta if v.startswith('- ')];added=[v[2:] for v in delta if v.startswith('+ ')]
            if len(removed)!=2 or len(added)!=2 or any(line.split('=',1)[0].strip() not in ('crash HTML','crash TXT') for line in removed+added):problems.append('config 진단 두 값 외 변경')
            for token in ['mailto:bugs@titanic.com','{troubleshooting URL}','troubleshootOnCrash','bugs@titanic.com']:
                if frozen.count(token)!=text.count(token):problems.append('진단 링크/키 변경:'+token)
            if re.findall(r'%[sd]',frozen)!=re.findall(r'%[sd]',text):problems.append('진단 서식 인자 변경')
            expected=[{'key':line.split('=',1)[0],'before':line,'after':added[index]} for index,line in enumerate(removed)]
        if record.get('sourceSha256') and record.get('source','').startswith(('webpj/','assets/')):
            source=ROOT/record['source'];sources.append({'path':record['source'],'expectedSha256':record['sourceSha256'],'currentSha256':sha(source)})
            if sha(source)!=record['sourceSha256']:problems.append('WB/기존 공유 입력 변경:'+record['source'])
        rows.append({'path':name,'frozenSource':str(original.relative_to(ROOT)).replace('\\','/'),'frozenSha256':sha(original),'finalSha256':sha(path),'bytes':len(raw),'differsFromFrozen':raw!=before})
    actual=[path for path in (FINAL/'assets/lang/korean').rglob('*') if path.is_file()]
    if len(actual)!=44:problems.append('최종 자산 수:'+str(len(actual)))
    if problems:raise ValueError('\n'.join(problems))
    return rows,sources,expected

# 함수 이름을 정적으로 수집할 뿐 검사 실행이나 컴파일을 호출하지 않는다.
def drafted_tests():
    """신규 검사 이름과 실행 필터를 후속 담당이 재사용할 수 있게 기록한다."""
    result=[]
    for name in ['cpppj/tests/DisplayTextInputTests.cpp','cpppj/tests/KoreanResourcesIntegrationTests.cpp','cpppj/tests/QuickHelpWindowTests.cpp']:
        path=FINAL/name;text=path.read_text('utf-8-sig')
        names=re.findall(r'TEST_CASE\(([^)]+)\)',text)
        if name.endswith('QuickHelpWindowTests.cpp'):names=[value for value in names if value.startswith('KoreanIntegration_')]
        result.append({'path':name,'names':names,'count':len(names),'compiled':False,'run':False})
    return result

# 후속 환경에서 경로별 기준/결과 해시와 전체 자산 사본을 확인할 수 있게 한다.
def main():
    """제품 변경 없이 최종 인계 manifest와 SHA 목록을 만든다."""
    code=code_patch();asset_rows,sources,diagnostics=assets();prior=prior_integrity()
    if prior:raise ValueError('이전 pending 변경:'+repr(prior))
    tests=drafted_tests();head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()
    report={'status':'적용 가능한 준비 후보/미적용/컴파일 및 실행 미검증','host':'HJOW-Athlon','lastDecompilation':'HJOW-Athlon/2026-11-03','productBaseHead':head,'initialProductHead':'4be0c4639ed6859d9ac438975b3839d70d44cb85','initialRawBase':'final-base-983','currentRawBase':'final-base','patch':'final.patch','patchSha256':sha(OUT/'final.patch'),'patchIncludesAssets':False,'codeFiles':len(code),'assetCount':len(asset_rows),'totalFinalFiles':len(code)+len(asset_rows),'draftTestCount':sum(row['count'] for row in tests),'builds':0,'cppTestsRun':0,'gameGuiImeMpRuns':0,'productFilesEdited':0,'fixturesEdited':0,'originalsEdited':0,'wbInputsEdited':0,'priorPendingIntegrityProblems':prior,'code':code,'assets':asset_rows,'preservedInputs':sources,'diagnosticChanges':diagnostics,'tests':tests,'frozenTranslationManifestSha256':sha(FROZEN/'combined-manifest.json'),'frozenTranslationShaListSha256':sha(FROZEN/'SHA256SUMS.txt'),'frozenIntegrationManifestSha256':sha(OUT/'source-manifest.json'),'frozenUiManifestSha256':sha(OUT.parent/'cpp-korean-ui-preparation/manifest.json'),'staticPatchCheck':'별도 git apply --check --ignore-space-change 명령의 final-static-verification.json 결과 참조'}
    (OUT/'final-manifest.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n','utf-8')
    lines=[row['finalSha256']+'  '+row['path'] for row in code+asset_rows]
    (OUT/'FINAL_SHA256SUMS.txt').write_text('\n'.join(lines)+'\n','utf-8')
    print(json.dumps({key:report[key] for key in ['productBaseHead','patchSha256','codeFiles','assetCount','totalFinalFiles','draftTestCount','priorPendingIntegrityProblems']},ensure_ascii=False,indent=2))

if __name__=='__main__':
    main()

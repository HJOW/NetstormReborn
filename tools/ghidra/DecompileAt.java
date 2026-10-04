// Ghidra 헤드리스 후처리 스크립트: 지정한 주소의 함수만 디컴파일해 C 파일로 내보낸다.
// 자동 분석이 함수로 인식하지 못한 코드(예: 가상 함수 표로만 호출되는 함수)는 그 주소에서 역어셈블하고
// 함수를 만들어 디컴파일한다. 프로젝트를 -readOnly 로 열면 이 변경은 저장되지 않는다.
//
// 사용 예 (tools/ghidra/decompile_at.ps1 참고):
//   analyzeHeadless <프로젝트폴더> Netstorm -process Netstorm.exe -noanalysis -readOnly
//       -scriptPath tools/ghidra -postScript DecompileAt.java <출력파일> 484ab0 4c2b20
//@category NetStorm

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;

import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;

public class DecompileAt extends GhidraScript {

    // 함수 하나당 디컴파일 제한 시간 (초). 메인 프레임 함수 FUN_004d62b0 처럼 거대한 함수가 60~120초로는 끝나지 않아 1시간으로 늘렸다
    private static final int DECOMPILE_TIMEOUT_SEC = 3600;

    // 스크립트 진입점: 첫 번째 인자는 출력 파일, 나머지는 16진수 주소 목록
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2) {
            printerr("사용법: DecompileAt.java <출력파일> <16진수 주소>...");
            return;
        }
        DecompInterface decomp = new DecompInterface();
        decomp.openProgram(currentProgram);
        try (PrintWriter out = new PrintWriter(new OutputStreamWriter(
                new FileOutputStream(args[0]), StandardCharsets.UTF_8))) {
            // 주소마다 함수를 찾거나 만들어 디컴파일
            for (int i = 1; i < args.length; i++) {
                Address entry = toAddr(Long.parseLong(args[i], 16));
                Function f = getFunctionAt(entry);
                if (f == null) {
                    // 자동 분석이 놓친 함수: 그 주소부터 역어셈블하고 함수를 만든다
                    disassemble(entry);
                    f = createFunction(entry, null);
                }
                if (f == null) {
                    out.println("// ==== " + args[i] + " 함수를 만들 수 없음");
                    continue;
                }
                out.println("// ==== " + f.getName() + " @ " + f.getEntryPoint());
                DecompileResults r = decomp.decompileFunction(f, DECOMPILE_TIMEOUT_SEC, monitor);
                if (r.decompileCompleted()) {
                    out.println(r.getDecompiledFunction().getC());
                } else {
                    out.println("// 디컴파일 실패: " + r.getErrorMessage());
                }
            }
        } finally {
            decomp.dispose();
        }
        println("주소 지정 디컴파일 완료: " + (args.length - 1) + "개");
    }
}

// Ghidra 헤드리스 후처리 스크립트: 프로그램의 모든 함수를 디컴파일해 하나의 C 파일로 내보낸다.
//
// 사용 예 (tools/ghidra/run_decomp.ps1 참고):
//   analyzeHeadless <프로젝트폴더> <프로젝트명> -import Netstorm.exe
//       -scriptPath tools/ghidra -postScript ExportDecomp.java <출력파일>
//@category NetStorm

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;

import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;

public class ExportDecomp extends GhidraScript {

    // 함수 하나당 디컴파일 제한 시간 (초)
    private static final int DECOMPILE_TIMEOUT_SEC = 60;

    // 스크립트 진입점: 첫 번째 인자로 받은 경로에 디컴파일 결과를 UTF-8 로 기록한다
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 1) {
            printerr("출력 파일 경로를 인자로 지정하세요.");
            return;
        }
        DecompInterface decomp = new DecompInterface();
        decomp.openProgram(currentProgram);
        int ok = 0;
        int fail = 0;
        try (PrintWriter out = new PrintWriter(new OutputStreamWriter(
                new FileOutputStream(args[0]), StandardCharsets.UTF_8))) {
            // 주소 순으로 모든 함수를 순회하며 디컴파일
            for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
                if (monitor.isCancelled()) {
                    break;
                }
                out.println("// ==== " + f.getName() + " @ " + f.getEntryPoint());
                DecompileResults r = decomp.decompileFunction(f, DECOMPILE_TIMEOUT_SEC, monitor);
                if (r.decompileCompleted()) {
                    out.println(r.getDecompiledFunction().getC());
                    ok++;
                } else {
                    out.println("// 디컴파일 실패: " + r.getErrorMessage());
                    fail++;
                }
            }
        } finally {
            decomp.dispose();
        }
        println("디컴파일 완료: 성공 " + ok + ", 실패 " + fail);
    }
}

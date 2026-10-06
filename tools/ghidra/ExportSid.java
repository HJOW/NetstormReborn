// SID 할당/반납의 원본 함수만 읽기 전용 프로젝트에서 다시 디컴파일하고 몸체 범위를 내보낸다.
//@category NetStorm
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.AddressRange;
import ghidra.program.model.listing.Function;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public class ExportSid extends GhidraScript {
    // 지정 함수의 디컴파일 제한 시간이다. 게임이나 GUI를 실행하지 않는다.
    private static final int TIMEOUT_SECONDS = 120;
    // 출력 폴더 뒤에 받은 함수 주소만 디컴파일한다. 프로젝트 변경은 저장하지 않는다.
    @Override public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2) throw new IllegalArgumentException("출력 폴더와 함수 주소가 필요합니다");
        Path dir = Path.of(args[0]);
        Files.createDirectories(dir);
        DecompInterface decomp = new DecompInterface();
        decomp.openProgram(currentProgram);
        try (PrintWriter c = new PrintWriter(Files.newBufferedWriter(dir.resolve("sid.c"), StandardCharsets.UTF_8));
             PrintWriter facts = new PrintWriter(Files.newBufferedWriter(dir.resolve("functions.tsv"), StandardCharsets.UTF_8))) {
            facts.println("entry\tranges");
            // 요청한 실제 함수의 범위를 각각 기록하여 격리 실행의 허용 코드로 사용한다.
            for (int i = 1; i < args.length; ++i) {
                Function f = getFunctionAt(toAddr(Long.parseLong(args[i], 16)));
                if (f == null) throw new IllegalStateException("함수가 없습니다: " + args[i]);
                StringBuilder ranges = new StringBuilder();
                // Ghidra가 확인한 불연속 몸체도 빠짐없이 기록한다.
                for (AddressRange r : f.getBody()) {
                    if (ranges.length() > 0) ranges.append(';');
                    ranges.append(r.getMinAddress()).append('-').append(r.getMaxAddress());
                }
                facts.println(f.getEntryPoint() + "\t" + ranges);
                DecompileResults result = decomp.decompileFunction(f, TIMEOUT_SECONDS, monitor);
                if (!result.decompileCompleted()) throw new IllegalStateException(result.getErrorMessage());
                c.println("// ==== " + f.getName() + " @ " + f.getEntryPoint());
                c.println(result.getDecompiledFunction().getC());
            }
        } finally { decomp.dispose(); }
        println("SID 디컴파일/몸체 범위 내보내기 완료: " + (args.length - 1));
    }
}

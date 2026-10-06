// 생성/Take/타입 초기화 함수를 읽기 전용 프로젝트에서 내보낸다.
//@category NetStorm
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressRange;
import ghidra.program.model.listing.Function;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public class ExportCreation extends GhidraScript {
    // 각 함수의 정적 디컴파일 제한 시간이다.
    private static final int TIMEOUT_SECONDS = 120;
    // 없는 CD 가상 함수는 메모리에서만 함수로 정의한다. -readOnly로 변경을 저장하지 않는다.
    @Override public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2) throw new IllegalArgumentException("출력 폴더와 함수 주소가 필요합니다");
        Path dir = Path.of(args[0]);
        Files.createDirectories(dir);
        DecompInterface decomp = new DecompInterface();
        decomp.openProgram(currentProgram);
        try (PrintWriter c = new PrintWriter(Files.newBufferedWriter(dir.resolve("creation.c"), StandardCharsets.UTF_8));
             PrintWriter facts = new PrintWriter(Files.newBufferedWriter(dir.resolve("functions.tsv"), StandardCharsets.UTF_8))) {
            facts.println("entry\tranges");
            // 각 시작 주소와 Ghidra가 계산한 불연속 몸체를 기록한다.
            for (int i = 1; i < args.length; ++i) {
                Address address = toAddr(Long.parseLong(args[i], 16));
                Function f = getFunctionAt(address);
                if (f == null) {
                    disassemble(address);
                    f = createFunction(address, null);
                }
                if (f == null) throw new IllegalStateException("함수 정의 실패: " + args[i]);
                StringBuilder ranges = new StringBuilder();
                // 비연속 주소를 한 덩어리로 확장하지 않는다.
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
        println("생성/Take 디컴파일 완료: " + (args.length - 1));
    }
}

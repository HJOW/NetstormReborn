// Ghidra 헤드리스 후처리 스크립트: 프로그램의 함수별 사실(범위·호출자·참조·반환 여부 등)을 TSV 로 내보낸다.
// 디컴파일 신뢰도 점검과 판본 간 함수 대응(tools/decomp_match.py)의 입력으로 쓴다. 프로그램은 바꾸지 않는다.
//
// 사용 예 (tools/ghidra/refine_decomp.ps1 참고):
//   analyzeHeadless <프로젝트폴더> Netstorm -process Netstorm.exe -noanalysis -readOnly
//       -scriptPath tools/ghidra -postScript DumpFunctions.java <함수 TSV 파일>
//@category NetStorm

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressRange;
import ghidra.program.model.address.AddressSetView;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.symbol.RefType;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;

import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.TreeSet;

public class DumpFunctions extends GhidraScript {

    // 한 칸에 적는 주소 목록의 최대 개수 (호출자가 수백 개인 함수의 줄이 지나치게 길어지지 않게 한다)
    private static final int MAX_LIST = 64;

    // 주소 집합을 "a,b,c" 형식의 16진수 문자열로 바꾼다 (최대 MAX_LIST 개)
    private String joinAddrs(TreeSet<Long> set) {
        StringBuilder sb = new StringBuilder();
        int n = 0;
        // 정렬된 주소를 앞에서부터 MAX_LIST 개까지만 적는다
        for (Long v : set) {
            if (n++ >= MAX_LIST) break;
            if (sb.length() > 0) sb.append(',');
            sb.append(Long.toHexString(v));
        }
        return sb.toString();
    }

    // 스크립트 진입점: 첫 번째 인자로 받은 경로에 함수별 사실을 UTF-8 TSV 로 기록한다
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 1) {
            printerr("사용법: DumpFunctions.java <함수 TSV 파일>");
            return;
        }
        int count = 0;
        try (PrintWriter out = new PrintWriter(new OutputStreamWriter(
                new FileOutputStream(args[0]), StandardCharsets.UTF_8))) {
            out.println("entry\tname\tsize\tranges\tnoreturn\tthunk\tcc\tcall_refs\tdata_refs\tjump_refs\t"
                    + "callers\tdata_ref_from\tjump_ref_from\tcalls\thas_ret\tindirect_jumps\t"
                    + "param_count\tsig_source\tsignature");
            // 주소 순으로 모든 함수를 순회한다
            for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
                if (monitor.isCancelled()) break;
                AddressSetView body = f.getBody();

                // 함수 몸체의 주소 범위 목록 ("시작-끝;시작-끝")
                List<String> ranges = new ArrayList<>();
                for (AddressRange r : body) {
                    ranges.add(Long.toHexString(r.getMinAddress().getOffset()) + "-"
                            + Long.toHexString(r.getMaxAddress().getOffset()));
                }

                // 진입점으로 들어오는 참조를 종류별로 나눈다 (직접 호출, 데이터 포인터, 몸체 밖에서 오는 점프)
                TreeSet<Long> callers = new TreeSet<>();
                TreeSet<Long> dataFrom = new TreeSet<>();
                TreeSet<Long> jumpFrom = new TreeSet<>();
                ReferenceIterator refs = currentProgram.getReferenceManager().getReferencesTo(f.getEntryPoint());
                while (refs.hasNext()) {
                    Reference ref = refs.next();
                    RefType t = ref.getReferenceType();
                    long from = ref.getFromAddress().getOffset();
                    if (t.isCall()) {
                        callers.add(from);
                    } else if (t.isJump()) {
                        if (!body.contains(ref.getFromAddress())) jumpFrom.add(from);
                    } else if (t.isData()) {
                        dataFrom.add(from);
                    }
                }

                // 몸체 안의 명령어를 훑어 호출 대상, ret 유무, 간접 점프 위치를 모은다
                TreeSet<Long> calls = new TreeSet<>();
                TreeSet<Long> indirect = new TreeSet<>();
                boolean hasRet = false;
                InstructionIterator it = currentProgram.getListing().getInstructions(body, true);
                while (it.hasNext()) {
                    Instruction ins = it.next();
                    String m = ins.getMnemonicString();
                    if (m.startsWith("RET")) hasRet = true;
                    if (ins.getFlowType().isCall()) {
                        // 직접 호출의 대상 주소만 기록한다 (간접 호출은 대상이 없다)
                        for (Address a : ins.getFlows()) calls.add(a.getOffset());
                    } else if (ins.getFlowType().isJump() && ins.getFlowType().isComputed()) {
                        indirect.add(ins.getAddress().getOffset());
                    }
                }

                out.println(Long.toHexString(f.getEntryPoint().getOffset()) + "\t" + f.getName() + "\t"
                        + body.getNumAddresses() + "\t" + String.join(";", ranges) + "\t"
                        + (f.hasNoReturn() ? 1 : 0) + "\t" + (f.isThunk() ? 1 : 0) + "\t"
                        + f.getCallingConventionName() + "\t"
                        + callers.size() + "\t" + dataFrom.size() + "\t" + jumpFrom.size() + "\t"
                        + joinAddrs(callers) + "\t" + joinAddrs(dataFrom) + "\t" + joinAddrs(jumpFrom) + "\t"
                        + joinAddrs(calls) + "\t" + (hasRet ? 1 : 0) + "\t" + joinAddrs(indirect) + "\t"
                        + f.getParameterCount() + "\t" + f.getSignatureSource() + "\t"
                        + f.getPrototypeString(false, true).replace('\t', ' '));
                count++;
            }
        }
        println("함수 사실 덤프 완료: " + count + "개");
    }
}

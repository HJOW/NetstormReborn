// Ghidra 헤드리스 후처리 스크립트: 자동 분석이 복구하지 못한 switch 점프 테이블을 exe 바이트에서 직접 읽어 고친다.
// 대상은 "jmp dword ptr [reg*4 + 표주소]" 형태의 간접 점프다. 표의 분기 대상이 그 함수 몸체에 들어 있지 않으면
// 점프 참조를 추가하고 함수 몸체를 다시 계산한다. 디컴파일 결과 자체는 바뀌지 않는다(디컴파일러는 표를 스스로 복구한다).
// 목적은 리스팅의 함수 몸체를 바로잡아, 분기 블록이 "함수 밖 코드"로 남아 가짜 함수로 복구되는 것을 막는 것이다.
// 예: 패치판 메인 프레임 함수 FUN_004d62b0 은 switch 3개의 분기 블록 46개가 몸체에서 빠져 있었다
//     (Decompiler Switch Analysis 분석기가 이 함수에서 제한 시간에 걸린 탓).
//
// 사용 예 (tools/ghidra/refine_decomp.ps1 참고):
//   analyzeHeadless <프로젝트폴더> Netstorm -process Netstorm.exe -noanalysis
//       -scriptPath tools/ghidra -postScript FixSwitches.java <switch 목록 TSV 파일>
//@category NetStorm

import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.mem.MemoryAccessException;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.scalar.Scalar;
import ghidra.program.model.symbol.RefType;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;
import ghidra.program.model.symbol.SourceType;

import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

public class FixSwitches extends GhidraScript {

    // 점프 테이블 항목 수의 상한 (경계 판별이 실패했을 때 무한히 읽지 않게 한다)
    private static final int MAX_ENTRIES = 1024;
    // 간접 점프 앞에서 범위 검사(cmp reg, N)를 찾을 때 거슬러 올라가는 명령어 수
    private static final int GUARD_LOOKBACK = 10;

    // 주소가 실행 가능한 초기화된 블록 안인지 확인한다
    private boolean inCode(Memory mem, Address a) {
        MemoryBlock blk = mem.getBlock(a);
        return blk != null && blk.isExecute() && blk.isInitialized();
    }

    // 명령어가 "jmp dword ptr [reg*4 + disp32]" 이면 표 주소를, 아니면 null 을 돌려준다
    private Address tableOf(Instruction ins) throws MemoryAccessException {
        byte[] b = ins.getBytes();
        // FF 24 SIB disp32 : SIB 가 scale=4, base=없음(101) 인 형태만 받는다
        if (b.length != 7 || (b[0] & 0xff) != 0xff || (b[1] & 0xff) != 0x24 || (b[2] & 0xc7) != 0x85) {
            return null;
        }
        long disp = (b[3] & 0xffL) | ((b[4] & 0xffL) << 8) | ((b[5] & 0xffL) << 16) | ((b[6] & 0xffL) << 24);
        return toAddr(disp);
    }

    // 간접 점프 앞의 명령어를 거슬러 올라가 범위 검사와 색인 표를 찾아 점프 테이블 항목 수를 정한다. 못 찾으면 -1
    private int entriesFromGuard(Memory mem, Instruction jmp) {
        Address indexTable = null;
        Instruction cur = jmp.getPrevious();
        // 최대 GUARD_LOOKBACK 개의 앞 명령어를 본다
        for (int i = 0; i < GUARD_LOOKBACK && cur != null; i++, cur = cur.getPrevious()) {
            String m = cur.getMnemonicString();
            if (m.equals("CMP")) {
                Object[] ops = cur.getOpObjects(1);
                if (ops.length == 1 && ops[0] instanceof Scalar) {
                    long n = ((Scalar) ops[0]).getUnsignedValue() + 1;
                    if (n < 1 || n > MAX_ENTRIES * 4) return -1;
                    if (indexTable == null) return (int) n;
                    // 2단 switch: 색인 표의 값 중 최댓값 + 1 이 점프 테이블 항목 수다
                    int max = -1;
                    try {
                        // 색인 표의 바이트 n 개를 읽어 최댓값을 구한다
                        for (long k = 0; k < n; k++) {
                            max = Math.max(max, mem.getByte(indexTable.add(k)) & 0xff);
                        }
                    } catch (MemoryAccessException ex) {
                        return -1;
                    }
                    return max + 1;
                }
                return -1;
            }
            if ((m.equals("MOV") || m.equals("MOVZX")) && indexTable == null) {
                // mov r8, byte ptr [reg + 색인표] 형태에서 색인 표 주소(코드 구간 안의 disp32)를 얻는다
                for (Object o : cur.getOpObjects(1)) {
                    Address cand = null;
                    if (o instanceof Address) cand = (Address) o;
                    else if (o instanceof Scalar) cand = toAddr(((Scalar) o).getUnsignedValue());
                    if (cand != null && inCode(mem, cand) && cur.getNumOperands() == 2) {
                        indexTable = cand;
                    }
                }
            }
            // 다른 분기를 만나면 이 점프의 범위 검사가 아니다 (ja/jbe 같은 조건 분기는 범위 검사의 일부라 허용)
            if (cur.getFlowType().isCall() || cur.getFlowType().isTerminal()) break;
        }
        return -1;
    }

    // 스크립트 진입점: 인자는 (선택) switch 목록 TSV 파일
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        Memory mem = currentProgram.getMemory();
        Listing listing = currentProgram.getListing();
        FunctionManager fm = currentProgram.getFunctionManager();

        // 1) 모든 switch 형태 간접 점프와 그 표 주소를 모은다 (표 주소 → 점프 명령어 주소)
        Map<Long, Address> tables = new TreeMap<>();
        List<Instruction> jumps = new ArrayList<>();
        InstructionIterator it = listing.getInstructions(true);
        while (it.hasNext()) {
            if (monitor.isCancelled()) return;
            Instruction ins = it.next();
            if (!ins.getMnemonicString().equals("JMP")) continue;
            Address table = tableOf(ins);
            if (table == null || !inCode(mem, table)) continue;
            tables.put(table.getOffset(), ins.getAddress());
            jumps.add(ins);
        }

        int fixed = 0, alreadyOk = 0, noFunction = 0, noTargets = 0, removedFns = 0;
        PrintWriter tsv = null;
        if (args.length >= 1) {
            tsv = new PrintWriter(new OutputStreamWriter(new FileOutputStream(args[0], true), StandardCharsets.UTF_8));
            tsv.println("jump\tfunction\ttable\tentries\ttargets\tmissing\taction");
        }
        try {
            // 2) 점프마다 표를 읽고, 분기 대상이 함수 몸체에 없으면 고친다
            for (Instruction ins : jumps) {
                if (monitor.isCancelled()) break;
                Address table = tableOf(ins);

                // 표 항목 읽기: 코드 구간 밖의 값이 나오거나 다른 표의 시작에 닿으면 멈춘다
                int guard = entriesFromGuard(mem, ins);
                List<Address> entries = new ArrayList<>();
                for (int i = 0; i < MAX_ENTRIES; i++) {
                    Address slot = table.add(4L * i);
                    if (i > 0 && tables.containsKey(slot.getOffset())) break;
                    if (guard > 0 && i >= guard) break;
                    Address target;
                    try {
                        target = toAddr(mem.getInt(slot) & 0xffffffffL);
                    } catch (MemoryAccessException ex) {
                        break;
                    }
                    if (!inCode(mem, target)) break;
                    entries.add(target);
                }
                LinkedHashSet<Address> targets = new LinkedHashSet<>(entries);

                Function fn = fm.getFunctionContaining(ins.getAddress());
                String action;
                int missing = 0;
                if (targets.size() < 2) {
                    noTargets++;
                    action = "skip-no-targets";
                } else if (fn == null) {
                    noFunction++;
                    action = "skip-no-function";
                } else {
                    // 이미 붙어 있는 점프 참조와 몸체 포함 여부를 확인한다
                    LinkedHashSet<Address> existing = new LinkedHashSet<>();
                    for (Reference ref : ins.getReferencesFrom()) {
                        if (ref.getReferenceType().isJump()) existing.add(ref.getToAddress());
                    }
                    // 몸체에 없거나 참조가 없는 분기 대상의 수
                    for (Address t : targets) {
                        if (!fn.getBody().contains(t) || !existing.contains(t)) missing++;
                    }
                    if (missing == 0) {
                        alreadyOk++;
                        action = "ok";
                    } else {
                        for (Address t : targets) {
                            // 분기 블록이 호출자 없는 별개 함수로 잘못 만들어져 있으면 그 함수를 지운다
                            Function other = fm.getFunctionAt(t);
                            if (other != null && !other.equals(fn)) {
                                boolean called = false;
                                ReferenceIterator refs = currentProgram.getReferenceManager().getReferencesTo(t);
                                while (refs.hasNext()) {
                                    if (refs.next().getReferenceType().isCall()) { called = true; break; }
                                }
                                if (!called) {
                                    fm.removeFunction(t);
                                    removedFns++;
                                }
                            }
                            if (listing.getInstructionAt(t) == null) disassemble(t);
                            if (!existing.contains(t)) {
                                ins.addOperandReference(0, t, RefType.COMPUTED_JUMP, SourceType.USER_DEFINED);
                            }
                        }
                        // 함수 몸체를 다시 계산해 분기 블록을 이 함수에 포함시킨다.
                        // 디컴파일러용 점프 테이블 재정의(JumpTable.writeOverride)는 쓰지 않는다: 디컴파일러는 표를 스스로
                        // 복구하며, 재정의를 쓰면 case 값이 실제 값이 아니라 0부터 매겨지고 같은 대상의 case 가 합쳐진다.
                        CreateFunctionCmd.fixupFunctionBody(currentProgram, fn, monitor);
                        fixed++;
                        action = "fixed";
                    }
                }
                if (tsv != null) {
                    tsv.println(Long.toHexString(ins.getAddress().getOffset()) + "\t"
                            + (fn == null ? "" : Long.toHexString(fn.getEntryPoint().getOffset())) + "\t"
                            + Long.toHexString(table.getOffset()) + "\t" + entries.size() + "\t" + targets.size()
                            + "\t" + missing + "\t" + action);
                }
            }
        } finally {
            if (tsv != null) tsv.close();
        }
        println("switch 점검 완료: 전체 " + jumps.size() + ", 고침 " + fixed + ", 정상 " + alreadyOk
                + ", 함수 밖 " + noFunction + ", 표 없음 " + noTargets + ", 지운 가짜 함수 " + removedFns);
    }
}

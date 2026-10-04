// Ghidra 헤드리스 후처리 스크립트: 기계어에서 직접 확인되는 근거만으로 함수의 호출 규약을 지정한다.
// 인자 목록은 잠그지 않는다(디컴파일러가 호출 규약을 바탕으로 계속 추정한다).
//
// 배경: Ghidra 의 "Decompiler Parameter ID"(DecompilerParameterIdCmd)로 원형을 일괄 확정하면 디컴파일러의 추측이
// 틀린 채로 굳어 호출자에게 전파된다. 실제로 MSVC C++ 코드인데 __fastcall 이 901개 나왔고, 가변 인자 로그 함수의
// 인자가 잘렸으며, extraout_ 변수가 있는 함수가 38개에서 311개로 늘었다. 그래서 그 방법은 쓰지 않는다.
//
// 판정 근거:
//   피호출 쪽: 함수가 ECX(또는 EDX)를 쓰기 전에 읽는다. "push ecx" 는 MSVC 가 지역 변수 4바이트를 잡는 데도 쓰므로
//              읽기로 치지 않는다. "xor ecx, ecx" 같은 0 만들기는 쓰기로 친다. 진입부터 ECX 를 건드리지 않고 다른
//              함수를 직접 호출하고 그 함수가 ECX 를 읽으면(this 를 그대로 넘기는 멤버 함수) 읽는 것으로 본다.
//   호출 쪽:   직접 호출 지점에서 바로 앞 호출 이후 ~ 이 호출 사이에 ECX(또는 EDX)를 쓴다. 호출자의 진입부터
//              이 호출까지 ECX 를 건드리지 않은 경우(통과 호출)도 ECX 를 넘긴 것으로 본다.
// 판정:
//   __thiscall  피호출 쪽과 호출 쪽 근거가 모두 있다 (호출 지점의 80% 이상).
//               또는 직접 호출자가 없고 데이터 포인터 참조가 있으며(가상 함수 표로만 호출) ECX 를 먼저 읽는다.
//   __fastcall  ECX 와 EDX 모두 양쪽 근거가 있다.
//   (지정 안 함) ECX 를 먼저 읽지만 호출 쪽 근거가 엇갈린다. 디컴파일러의 자체 추정에 맡기고, 상대 판본의 근거로
//               보완할 후보로 남긴다 (basis = ambiguous-ecx).
//   __stdcall   ECX 를 먼저 읽지 않고 ret N (N>0) 이다.
//   __cdecl     그 밖.
// 두 번째 인자로 힌트 파일(주소<TAB>규약)을 주면 그 함수는 힌트의 규약으로 지정한다
// (tools/decomp_refine.py 가 상대 판본의 근거로 모호한 함수를 보완해 만든다).
//
// 사용 예 (tools/ghidra/refine_decomp.ps1 참고):
//   analyzeHeadless <프로젝트폴더> Netstorm -process Netstorm.exe -noanalysis
//       -scriptPath tools/ghidra -postScript ApplyConventions.java <근거 TSV 파일> [<힌트 TSV 파일>]
//@category NetStorm

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.block.BasicBlockModel;
import ghidra.program.model.block.CodeBlock;
import ghidra.program.model.block.CodeBlockReference;
import ghidra.program.model.block.CodeBlockReferenceIterator;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.symbol.FlowType;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;
import ghidra.program.model.symbol.SourceType;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;

public class ApplyConventions extends GhidraScript {

    // 함수 진입부에서 레지스터의 첫 사용을 찾을 때 보는 최대 명령어 수
    private static final int ENTRY_SCAN = 96;
    // 호출 지점에서 거슬러 올라가며 레지스터 설정을 찾을 때 보는 최대 명령어 수
    private static final int CALLSITE_SCAN = 64;
    // 호출 쪽 근거로 인정하는 최소 비율 (레지스터를 넘긴 호출 지점 / 전체 직접 호출 지점)
    private static final double CALLER_RATIO = 0.8;
    // 통과 호출 전파(진입부터 ECX 를 건드리지 않고 넘기는 함수)를 반복하는 최대 횟수
    private static final int MAX_PROPAGATE = 32;

    // 레지스터 첫 사용 판정 결과: 쓰기가 먼저이거나 쓰이지 않음
    private static final int FIRST_WRITE_OR_NONE = 0;
    // 레지스터 첫 사용 판정 결과: 읽기가 먼저
    private static final int FIRST_READ = 1;
    // 레지스터 첫 사용 판정 결과: 건드리기 전에 다른 함수를 호출함 (통과 호출)
    private static final int FIRST_CALL = 2;

    // 스택 프로브 함수(__chkstk / _alloca_probe)의 시작 바이트 두 가지.
    // 큰 지역 변수를 쓰는 함수는 "mov eax, 크기; call __chkstk" 로 시작한다. 이 함수는 ECX·EDX 를 보존하므로
    // 레지스터 추적에서 호출로 치지 않는다. CD판에서는 Ghidra 가 이름을 붙이지 못해 바이트로 식별한다.
    //   CD판(0x4f1bd0):   push ecx; cmp eax, 0x1000; lea ecx, [esp+8]
    //   패치판(0x4e4a40): cmp eax, 0x1000; jae +0x0e; neg eax; add eax, esp
    private static final int[][] STACK_PROBE_BYTES = {
        {0x51, 0x3d, 0x00, 0x10, 0x00, 0x00, 0x8d, 0x4c, 0x24, 0x08},
        {0x3d, 0x00, 0x10, 0x00, 0x00, 0x73, 0x0e, 0xf7, 0xd8, 0x03, 0xc4},
    };

    // 진입 시점 생존 여부를 따질 때 한 함수에서 방문하는 기본 블록의 최대 개수
    private static final int MAX_BLOCKS = 600;

    private Register ecx;
    private Register edx;
    // 함수 안의 흐름을 따라가기 위한 기본 블록 모델
    private BasicBlockModel blockModel;
    // 주소 → 스택 프로브 함수 여부 (같은 대상을 여러 번 읽지 않게 기억한다)
    private final Map<Long, Boolean> probeCache = new HashMap<>();

    // 호출 명령어의 대상이 스택 프로브 함수인지 확인한다
    private boolean callsStackProbe(Instruction call) {
        Address[] flows = call.getFlows();
        if (flows.length != 1) return false;
        Address target = flows[0];
        Boolean known = probeCache.get(target.getOffset());
        if (known != null) return known;
        boolean probe = false;
        // 알려진 시작 바이트 형태를 하나씩 대조한다
        for (int[] pattern : STACK_PROBE_BYTES) {
            boolean same = true;
            try {
                // 대상 주소의 첫 바이트들이 이 형태와 같은지 비교한다
                for (int i = 0; i < pattern.length; i++) {
                    if ((currentProgram.getMemory().getByte(target.add(i)) & 0xff) != pattern[i]) {
                        same = false;
                        break;
                    }
                }
            } catch (Exception ex) {
                same = false;
            }
            if (same) {
                probe = true;
                break;
            }
        }
        probeCache.put(target.getOffset(), probe);
        return probe;
    }

    // 객체 배열 안에 주어진 레지스터(또는 그 일부: CL, CX 등)가 있는지 확인한다
    private boolean touches(Object[] objects, Register base) {
        // 명령어가 읽거나 쓰는 대상 가운데 레지스터만 본다
        for (Object o : objects) {
            if (o instanceof Register) {
                Register r = (Register) o;
                if (r.equals(base) || base.equals(r.getBaseRegister()) || base.contains(r)) return true;
            }
        }
        return false;
    }

    // 명령어가 "xor r, r" / "sub r, r" 처럼 레지스터를 0 으로 만드는 관용구인지 확인한다 (읽기가 아니라 쓰기다)
    private boolean isZeroIdiom(Instruction ins, Register base) {
        String m = ins.getMnemonicString();
        if (!(m.equals("XOR") || m.equals("SUB")) || ins.getNumOperands() != 2) return false;
        Object[] a = ins.getOpObjects(0);
        Object[] b = ins.getOpObjects(1);
        return a.length == 1 && b.length == 1 && a[0].equals(b[0]) && touches(a, base);
    }

    // 함수 진입부를 주소 순으로 훑어 레지스터의 첫 사용을 판정한다.
    // 건드리기 전에 직접 호출을 만나면 FIRST_CALL 을 돌려주고 callOut[0] 에 호출 대상을 적는다
    private int firstUse(Function f, Register base, Address[] callOut) {
        Listing listing = currentProgram.getListing();
        Instruction ins = listing.getInstructionAt(f.getEntryPoint());
        // 진입점부터 최대 ENTRY_SCAN 개의 명령어를 본다
        for (int i = 0; i < ENTRY_SCAN && ins != null; i++, ins = ins.getNext()) {
            if (!f.getBody().contains(ins.getAddress())) break;
            FlowType flow = ins.getFlowType();
            // push 는 지역 변수 공간을 잡는 용도일 수 있어 읽기로 치지 않는다
            boolean neutral = ins.getMnemonicString().equals("PUSH");
            if (!neutral) {
                if (isZeroIdiom(ins, base)) return FIRST_WRITE_OR_NONE;
                if (touches(ins.getInputObjects(), base)) return FIRST_READ;
                if (touches(ins.getResultObjects(), base)) return FIRST_WRITE_OR_NONE;
            }
            if (flow.isCall()) {
                // 스택 프로브 호출은 ECX/EDX 를 보존하므로 건너뛰고 계속 본다
                if (callsStackProbe(ins)) continue;
                // 호출은 ECX/EDX 를 망가뜨린다. 그 전에 건드리지 않았으면 레지스터 값이 그대로 넘어간다
                Address[] flows = ins.getFlows();
                if (callOut != null && flows.length == 1) callOut[0] = flows[0];
                return FIRST_CALL;
            }
            // 반환·무조건 점프에서 멈춘다
            if (flow.isTerminal() || (flow.isJump() && !flow.isConditional())) break;
        }
        return FIRST_WRITE_OR_NONE;
    }

    // 기본 블록 하나를 훑어 레지스터의 운명을 판정한다: 읽으면 FIRST_READ, 쓰거나 호출로 망가지면 FIRST_CALL,
    // 건드리지 않고 블록 끝까지 가면 FIRST_WRITE_OR_NONE(다음 블록으로 계속)
    private int scanBlock(CodeBlock block, Register base) {
        InstructionIterator it = currentProgram.getListing().getInstructions(block, true);
        // 블록 안의 명령어를 주소 순으로 본다
        while (it.hasNext()) {
            Instruction ins = it.next();
            boolean neutral = ins.getMnemonicString().equals("PUSH");
            if (!neutral) {
                if (isZeroIdiom(ins, base)) return FIRST_CALL;
                if (touches(ins.getInputObjects(), base)) return FIRST_READ;
                if (touches(ins.getResultObjects(), base)) return FIRST_CALL;
            }
            // 스택 프로브가 아닌 호출은 레지스터를 망가뜨린다
            if (ins.getFlowType().isCall() && !callsStackProbe(ins)) return FIRST_CALL;
        }
        return FIRST_WRITE_OR_NONE;
    }

    // 함수 진입 시점에 레지스터 값이 살아 있는지(쓰기·호출 없이 읽기에 도달하는 경로가 있는지) 기본 블록을 따라 확인한다.
    // 직선 스캔(firstUse)은 분기 뒤의 경로에서만 읽는 함수를 놓치므로 이 방법으로 보완한다
    private boolean liveAtEntry(Function f, Register base) throws Exception {
        CodeBlock start = blockModel.getCodeBlockAt(f.getEntryPoint(), monitor);
        if (start == null) return false;
        ArrayDeque<CodeBlock> work = new ArrayDeque<>();
        Set<Long> seen = new HashSet<>();
        work.add(start);
        int visited = 0;
        // 진입 블록에서 시작해 레지스터가 아직 건드려지지 않은 블록만 따라간다
        while (!work.isEmpty() && visited < MAX_BLOCKS) {
            CodeBlock block = work.poll();
            if (!seen.add(block.getFirstStartAddress().getOffset())) continue;
            visited++;
            int fate = scanBlock(block, base);
            if (fate == FIRST_READ) return true;
            if (fate == FIRST_CALL) continue;
            CodeBlockReferenceIterator dests = block.getDestinations(monitor);
            // 호출이 아닌 흐름(점프, 다음 블록)만 따라가고 함수 몸체 밖으로는 나가지 않는다
            while (dests.hasNext()) {
                CodeBlockReference ref = dests.next();
                if (ref.getFlowType().isCall()) continue;
                CodeBlock dest = ref.getDestinationBlock();
                if (dest != null && f.getBody().contains(dest.getFirstStartAddress())) work.add(dest);
            }
        }
        return false;
    }

    // 호출 명령어 앞을 거슬러 올라가 이 호출에 레지스터가 넘어가는지 확인한다.
    // 바로 앞 호출 이후에 레지스터를 쓰거나, 호출자의 진입부터 건드리지 않고 그대로 온 경우(통과 호출)에 true
    private boolean passedToCall(Instruction call, Register base, Function caller) {
        Instruction ins = call.getPrevious();
        // 최대 CALLSITE_SCAN 개의 앞 명령어를 본다
        for (int i = 0; i < CALLSITE_SCAN && ins != null; i++, ins = ins.getPrevious()) {
            // 호출자의 진입점보다 앞으로 넘어가면 진입부터 건드리지 않은 것이다
            if (caller != null && ins.getAddress().compareTo(caller.getEntryPoint()) < 0) return true;
            FlowType flow = ins.getFlowType();
            // 스택 프로브 호출은 ECX/EDX 를 보존하므로 건너뛰고 계속 거슬러 올라간다
            if (flow.isCall() && callsStackProbe(ins)) continue;
            // 앞 호출·반환·무조건 점프를 만나면 그 앞은 이 호출과 무관하다
            if (flow.isCall() || flow.isTerminal() || (flow.isJump() && !flow.isConditional())) return false;
            if (touches(ins.getResultObjects(), base)) return true;
        }
        return false;
    }

    // 힌트 파일(주소<TAB>규약)을 읽는다. 파일이 없으면 빈 표
    private Map<Long, String> readHints(String path) throws Exception {
        Map<Long, String> hints = new HashMap<>();
        if (path == null) return hints;
        try (BufferedReader in = new BufferedReader(new InputStreamReader(new FileInputStream(path),
                StandardCharsets.UTF_8))) {
            String line;
            // 머리줄과 빈 줄을 건너뛰고 한 줄에 한 함수씩 읽는다
            while ((line = in.readLine()) != null) {
                String[] cols = line.trim().split("\t");
                if (cols.length < 2 || cols[0].equals("entry")) continue;
                hints.put(Long.parseLong(cols[0], 16), cols[1]);
            }
        }
        return hints;
    }

    // 스크립트 진입점: 인자는 근거 TSV 파일, (선택) 힌트 TSV 파일
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 1) {
            printerr("사용법: ApplyConventions.java <근거 TSV 파일> [<힌트 TSV 파일>]");
            return;
        }
        ecx = currentProgram.getRegister("ECX");
        edx = currentProgram.getRegister("EDX");
        blockModel = new BasicBlockModel(currentProgram);
        Map<Long, String> hints = readHints(args.length >= 2 ? args[1] : null);
        Listing listing = currentProgram.getListing();
        FunctionManager fm = currentProgram.getFunctionManager();

        // 1) 피호출 쪽 근거: 함수마다 ECX, EDX 의 첫 사용을 판정한다
        List<Function> functions = new ArrayList<>();
        Set<Long> ecxIn = new HashSet<>();
        Set<Long> edxIn = new HashSet<>();
        Map<Long, Long> passTarget = new HashMap<>();
        for (Function f : fm.getFunctions(true)) {
            if (monitor.isCancelled()) return;
            if (f.isThunk() || f.isExternal()) continue;
            functions.add(f);
            long entry = f.getEntryPoint().getOffset();
            Address[] callOut = new Address[1];
            int first = firstUse(f, ecx, callOut);
            if (first == FIRST_READ || liveAtEntry(f, ecx)) {
                ecxIn.add(entry);
            } else if (first == FIRST_CALL && callOut[0] != null) {
                passTarget.put(entry, callOut[0].getOffset());
            }
            if (firstUse(f, edx, null) == FIRST_READ || liveAtEntry(f, edx)) edxIn.add(entry);
        }
        // 통과 호출 전파: 진입부터 ECX 를 건드리지 않고 부른 함수가 ECX 를 읽으면 이 함수도 ECX 를 받는다
        Set<Long> passThrough = new HashSet<>();
        for (int round = 0; round < MAX_PROPAGATE; round++) {
            int added = 0;
            // 통과 호출의 대상이 ECX 를 받는 함수로 판정됐는지 다시 확인한다
            for (Map.Entry<Long, Long> e : passTarget.entrySet()) {
                if (!ecxIn.contains(e.getKey()) && ecxIn.contains(e.getValue())) {
                    ecxIn.add(e.getKey());
                    passThrough.add(e.getKey());
                    added++;
                }
            }
            if (added == 0) break;
        }

        Map<String, Integer> decided = new TreeMap<>();
        int hinted = 0;
        try (PrintWriter out = new PrintWriter(new OutputStreamWriter(
                new FileOutputStream(args[0]), StandardCharsets.UTF_8))) {
            out.println("entry\tecx_in\tedx_in\tcall_sites\tecx_set\tedx_set\tdata_refs\tpurge\tprevious\tdecision\tbasis");
            // 2) 호출 쪽 근거를 모아 함수마다 판정한다
            for (Function f : functions) {
                if (monitor.isCancelled()) break;
                Address entry = f.getEntryPoint();
                long key = entry.getOffset();
                String previous = f.getCallingConventionName();
                boolean readsEcx = ecxIn.contains(key);
                boolean readsEdx = edxIn.contains(key);

                // 직접 호출 지점마다 ECX, EDX 가 넘어가는지 확인하고, 데이터 포인터 참조 수도 센다
                int sites = 0, ecxSet = 0, edxSet = 0, dataRefs = 0;
                ReferenceIterator refs = currentProgram.getReferenceManager().getReferencesTo(entry);
                while (refs.hasNext()) {
                    Reference ref = refs.next();
                    if (ref.getReferenceType().isData()) {
                        dataRefs++;
                        continue;
                    }
                    if (!ref.getReferenceType().isCall()) continue;
                    Instruction call = listing.getInstructionAt(ref.getFromAddress());
                    if (call == null) continue;
                    Function caller = fm.getFunctionContaining(ref.getFromAddress());
                    sites++;
                    if (passedToCall(call, ecx, caller)) ecxSet++;
                    if (passedToCall(call, edx, caller)) edxSet++;
                }

                // ret N 의 N (피호출 함수가 정리하는 스택 인자 크기). 알 수 없으면 -1
                int purge = f.isStackPurgeSizeValid() ? f.getStackPurgeSize() : -1;

                // 판정: 근거가 강한 순서로 정한다
                String decision;
                String basis;
                boolean ecxCallers = sites > 0 && ecxSet >= sites * CALLER_RATIO;
                boolean edxCallers = sites > 0 && edxSet >= sites * CALLER_RATIO;
                if (hints.containsKey(key)) {
                    decision = hints.get(key);
                    basis = "hint";
                    hinted++;
                } else if (readsEcx && readsEdx && ecxCallers && edxCallers) {
                    decision = "__fastcall";
                    basis = "ecx+edx-both-sides";
                } else if (readsEcx && ecxCallers) {
                    decision = "__thiscall";
                    basis = passThrough.contains(key) ? "ecx-pass-through" : "ecx-both-sides";
                } else if (readsEcx && sites == 0 && dataRefs > 0) {
                    decision = "__thiscall";
                    basis = "ecx-callee+pointer";
                } else if (readsEcx) {
                    decision = "unknown";
                    basis = "ambiguous-ecx";
                } else if (purge > 0) {
                    decision = "__stdcall";
                    basis = "ret-n";
                } else {
                    decision = "__cdecl";
                    basis = purge == 0 ? "ret-0" : "default";
                }

                // 사용자·가져온 정보로 이미 정해진 원형은 건드리지 않는다. "unknown" 은 지정하지 않는다는 뜻이다
                if (f.getSignatureSource() == SourceType.USER_DEFINED || f.getSignatureSource() == SourceType.IMPORTED) {
                    basis = "kept-" + f.getSignatureSource();
                    decision = previous;
                } else if (!decision.equals(previous)) {
                    // 다시 실행해도 같은 결과가 나오도록, 이전 실행이 지정한 규약도 "unknown" 으로 되돌린다
                    f.setCallingConvention(decision);
                }
                decided.merge(decision, 1, Integer::sum);
                out.println(Long.toHexString(key) + "\t" + (readsEcx ? 1 : 0) + "\t" + (readsEdx ? 1 : 0)
                        + "\t" + sites + "\t" + ecxSet + "\t" + edxSet + "\t" + dataRefs + "\t" + purge + "\t"
                        + previous + "\t" + decision + "\t" + basis);
            }
        }
        println("호출 규약 지정 완료: " + decided + ", 통과 호출로 ECX 를 받는 함수 " + passThrough.size()
                + ", 힌트 적용 " + hinted);
    }
}

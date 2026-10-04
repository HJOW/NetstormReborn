// Ghidra 헤드리스 후처리 스크립트: 자동 분석이 함수로 인식하지 못한 코드를 모두 찾아 함수로 만들고 디컴파일한다.
// 후보는 다음 방법으로 모으며, 새 함수를 만들면 그 뒤에 이어진 코드가 새 후보가 되므로 더 나오지 않을 때까지 반복한다.
//   0)  직접 call 의 대상인데 함수가 없는 주소 (출처 call)
//   1)  코드 구간에서 어떤 함수에도 속하지 않고, 바로 앞이 패딩(INT3, NOP, mov edi,edi, lea reg,[reg+0])이며,
//       흔한 함수 프롤로그로 시작하는 주소 (출처 prologue). 패딩 명령어 자체는 함수 시작으로 보지 않는다
//   1b) 기존 함수의 끝 바로 다음 주소(패딩 없이 이어진 함수)가 프롤로그로 시작하는 경우 (출처 adjacent)
//   1c) 함수 밖 구간에서 ret / ret N / jmp rel32 바로 뒤에 패딩 없이 프롤로그가 이어지는 경우 (출처 afterret)
//   2)  코드가 아닌 구간(.rdata/.data)에 4바이트 값으로 저장된, 코드 구간 안의 주소 (출처 pointer: 가상 함수 표·콜백 표)
// 프로젝트를 -readOnly 로 열면 만든 함수는 저장되지 않는다. 정밀 디컴파일(refine_decomp.ps1)은 "commit" 인자로
// 프로젝트에 함수를 실제로 만들고 저장한다.
//
// 사용 예 (tools/ghidra/refine_decomp.ps1, tools/ghidra/run_script.ps1 참고):
//   analyzeHeadless <프로젝트폴더> Netstorm -process Netstorm.exe -noanalysis -readOnly
//       -scriptPath tools/ghidra -postScript RecoverMissing.java <결과 C 파일|-> <후보 목록 TSV 파일>
//           [<함수 밖 코드 구간 TSV 파일>] [commit]
//@category NetStorm

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.address.AddressIterator;
import ghidra.program.model.address.AddressSet;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
import ghidra.program.model.mem.Memory;
import ghidra.program.model.mem.MemoryAccessException;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.ReferenceIterator;

import java.io.FileOutputStream;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

public class RecoverMissing extends GhidraScript {

    // 함수 하나당 디컴파일 제한 시간 (초)
    private static final int DECOMPILE_TIMEOUT_SEC = 3600;

    // 후보 수집·함수 생성 반복의 최대 라운드 수
    private static final int MAX_ROUNDS = 8;

    // 후보 출처 표시: 프롤로그 휴리스틱 (앞이 CC 패딩)
    private static final int SRC_PROLOGUE = 1;
    // 후보 출처 표시: 데이터 구간의 코드 포인터
    private static final int SRC_POINTER = 2;
    // 후보 출처 표시: 기존 함수 끝 바로 다음 주소 (패딩 없이 이어진 함수)
    private static final int SRC_ADJACENT = 4;
    // 후보 출처 표시: ret/jmp 직후 (패딩 없이 이어진 함수가 함수 밖 구간에 있는 경우)
    private static final int SRC_AFTER_RET = 8;
    // 후보 출처 표시: 직접 call 의 대상인데 함수가 없는 주소 (가장 강한 근거)
    private static final int SRC_CALL = 16;

    // 이번 라운드의 후보 주소 → 출처 비트 집합 (주소 순 정렬)
    private final Map<Long, Integer> candidates = new TreeMap<>();
    // 후보 주소 → 그 주소를 가리키는 데이터 포인터 개수
    private final Map<Long, Integer> pointerCounts = new TreeMap<>();

    // 바이트를 읽을 수 없는 위치(블록 끝 근처)는 프롤로그가 아닌 것으로 본다
    private boolean looksLikePrologue(Memory mem, Address a) {
        try {
            return looksLikePrologueUnsafe(mem, a);
        } catch (Exception ex) {
            return false;
        }
    }

    // 바이트가 흔한 함수 프롤로그의 시작인지 판별한다 (push ebp, sub esp, push reg, mov reg 등)
    private boolean looksLikePrologueUnsafe(Memory mem, Address a) throws Exception {
        int b0 = mem.getByte(a) & 0xff;
        int b1 = mem.getByte(a.add(1)) & 0xff;
        // 55 8B EC / 55 89 E5 : push ebp; mov ebp, esp
        if (b0 == 0x55 && (b1 == 0x8b || b1 == 0x89)) return true;
        // 83 EC xx / 81 EC xx xx xx xx : sub esp, N
        if (b0 == 0x83 && b1 == 0xec) return true;
        if (b0 == 0x81 && b1 == 0xec) return true;
        // (8B FF "mov edi, edi" 는 프롤로그가 아니라 2바이트 정렬 패딩이므로 여기서 받지 않는다 — paddingLength 참고)
        // B9 imm32 뒤에 E8(call)/E9(jmp) : mov ecx, 객체 주소; call 생성자 (정적 초기화·소멸 스텁)
        if (b0 == 0xb9 && ((mem.getByte(a.add(5)) & 0xff) == 0xe8 || (mem.getByte(a.add(5)) & 0xff) == 0xe9)) return true;
        // 64 A1 00 00 00 00 : mov eax, fs:[0] (SEH 프롤로그의 다른 형태)
        if (b0 == 0x64 && b1 == 0xa1) return true;
        // 6A FF : push -1 (SEH 프롤로그)
        if (b0 == 0x6a && b1 == 0xff) return true;
        // 50~57 : push reg
        if (b0 >= 0x50 && b0 <= 0x57) return true;
        // 8B 44/4C/54 24 xx : mov reg, [esp+N] (스택 인자를 바로 읽는 작은 함수)
        if (b0 == 0x8b && (b1 == 0x44 || b1 == 0x4c || b1 == 0x54 || b1 == 0x5c || b1 == 0x74 || b1 == 0x7c)
                && (mem.getByte(a.add(2)) & 0xff) == 0x24) return true;
        // 8B C1 / 8B F1 / 8B D1 : mov eax, ecx 등 (thiscall 작은 함수)
        if (b0 == 0x8b && (b1 == 0xc1 || b1 == 0xf1 || b1 == 0xd1 || b1 == 0xf9 || b1 == 0xc9)) return true;
        // 31 C0 / 33 C0 / 32 C0 : xor eax, eax 로 시작하는 상수 반환 함수
        if ((b0 == 0x31 || b0 == 0x33 || b0 == 0x32) && b1 == 0xc0) return true;
        // B8 imm32 : mov eax, imm 으로 시작하는 함수
        if (b0 == 0xb8) return true;
        // C2 / C3 : 곧바로 반환하는 빈 함수
        if (b0 == 0xc3 || b0 == 0xc2) return true;
        // A1 / 8B 0D / 8B 15 : 전역 변수 읽기로 시작하는 함수
        if (b0 == 0xa1) return true;
        if (b0 == 0x8b && (b1 == 0x0d || b1 == 0x15 || b1 == 0x05)) return true;
        // 83 7C 24 / 80 7C 24 / 83 79 / 83 78 : cmp 로 시작하는 함수
        if ((b0 == 0x83 || b0 == 0x80) && (b1 == 0x7c || b1 == 0x79 || b1 == 0x78 || b1 == 0x7b)) return true;
        // E9 : jmp thunk
        if (b0 == 0xe9) return true;
        // FF 25 : jmp [import] thunk
        if (b0 == 0xff && b1 == 0x25) return true;
        return false;
    }

    // 출처 비트 집합을 사람이 읽는 문자열로 바꾼다 (예: "prologue+pointer")
    private String sourceName(int s) {
        List<String> names = new ArrayList<>();
        if ((s & SRC_CALL) != 0) names.add("call");
        if ((s & SRC_PROLOGUE) != 0) names.add("prologue");
        if ((s & SRC_ADJACENT) != 0) names.add("adjacent");
        if ((s & SRC_AFTER_RET) != 0) names.add("afterret");
        if ((s & SRC_POINTER) != 0) names.add("pointer");
        return String.join("+", names);
    }

    // 후보 하나를 기록한다 (이미 있으면 출처 비트를 합친다)
    private void addCandidate(Address a, int source) {
        candidates.merge(a.getOffset(), source, (x, y) -> x | y);
    }

    // 주소가 코드 블록 안에 있는지 확인한다
    private boolean inCodeBlocks(List<MemoryBlock> codeBlocks, Address a) {
        for (MemoryBlock cb : codeBlocks) {
            if (cb.contains(a)) return true;
        }
        return false;
    }

    // 주소에 놓인 패딩 명령어의 길이를 돌려준다 (패딩이 아니면 0).
    // 함수 사이·정렬용 패딩: INT3(CC), NOP(90), mov edi,edi(8B FF), lea reg,[reg+0] 형태 다바이트 NOP.
    // CD판 컴파일러는 INT3 대신 8B FF, 8D 49 00, 8D 64 24 00, 8D 9B 00000000, 8D A4 24 00000000 을 이어 붙여 채운다.
    private int paddingLength(Memory mem, Address a) {
        try {
            int b0 = mem.getByte(a) & 0xff;
            if (b0 == 0xcc || b0 == 0x90) return 1;
            int b1 = mem.getByte(a.add(1)) & 0xff;
            if (b0 == 0x8b && b1 == 0xff) return 2;
            if (b0 != 0x8d) return 0;
            // lea reg, [reg + 0] : ModRM 의 reg 와 rm 이 같은 레지스터이고 변위가 0 이어야 아무 일도 하지 않는다
            int mod = b1 >> 6;
            int reg = (b1 >> 3) & 7;
            int rm = b1 & 7;
            if (reg != rm) return 0;
            if (rm == 4) {
                // esp 는 SIB 바이트 0x24 (base=esp, index 없음)가 뒤따른다
                if ((mem.getByte(a.add(2)) & 0xff) != 0x24) return 0;
                if (mod == 1 && mem.getByte(a.add(3)) == 0) return 4;
                if (mod == 2 && mem.getInt(a.add(3)) == 0) return 7;
                return 0;
            }
            if (mod == 1 && mem.getByte(a.add(2)) == 0) return 3;
            if (mod == 2 && mem.getInt(a.add(2)) == 0) return 6;
        } catch (MemoryAccessException ex) {
            return 0;
        }
        return 0;
    }

    // 주소 바로 앞이 패딩 명령어로 끝나는지 확인한다
    private boolean afterPadding(Memory mem, Address a) {
        // 가능한 패딩 길이마다 그만큼 앞에서 시작하는 패딩 명령어가 있는지 본다
        for (int len : new int[] {1, 2, 3, 4, 6, 7}) {
            try {
                if (paddingLength(mem, a.subtract(len)) == len) return true;
            } catch (Exception ex) {
                // 블록 맨 앞 근처에서는 앞 주소가 없을 수 있다
            }
        }
        return false;
    }

    // 바로 앞이 함수를 끝내는 명령(ret, ret N, jmp rel32)인지 확인한다
    private boolean afterTerminator(Memory mem, Address a) {
        try {
            if ((mem.getByte(a.subtract(1)) & 0xff) == 0xc3) return true;
            if ((mem.getByte(a.subtract(3)) & 0xff) == 0xc2 && mem.getByte(a.subtract(1)) == 0) return true;
            if ((mem.getByte(a.subtract(5)) & 0xff) == 0xe9) return true;
        } catch (MemoryAccessException ex) {
            return false;
        }
        return false;
    }

    // 현재 함수 상태에서 누락 함수 후보를 모두 모은다
    private void collectCandidates(Memory mem, FunctionManager fm, List<MemoryBlock> codeBlocks) throws Exception {
        // 0) 직접 call 의 대상인데 함수가 없는 주소: 새로 역어셈블된 코드가 부르는 함수가 여기서 드러난다
        AddressSet codeSet = new AddressSet();
        for (MemoryBlock blk : codeBlocks) codeSet.add(blk.getStart(), blk.getEnd());
        AddressIterator dests = currentProgram.getReferenceManager().getReferenceDestinationIterator(codeSet, true);
        while (dests.hasNext()) {
            if (monitor.isCancelled()) return;
            Address d = dests.next();
            if (fm.getFunctionContaining(d) != null) continue;
            // 이 주소로 들어오는 참조 가운데 call 이 하나라도 있으면 후보다
            ReferenceIterator refs = currentProgram.getReferenceManager().getReferencesTo(d);
            while (refs.hasNext()) {
                if (refs.next().getReferenceType().isCall()) {
                    addCandidate(d, SRC_CALL);
                    break;
                }
            }
        }

        // 1) 프롤로그 휴리스틱: 어떤 함수에도 속하지 않고 앞 바이트가 CC 이며 프롤로그로 시작하는 주소
        //    (1c: 앞이 ret/jmp 로 끝나 패딩 없이 이어지는 경우도 같은 규칙으로 후보에 넣는다)
        for (MemoryBlock blk : codeBlocks) {
            Address a = blk.getStart().add(5);
            Address end = blk.getEnd().subtract(2);
            while (a.compareTo(end) < 0) {
                if (monitor.isCancelled()) return;
                int cur = mem.getByte(a) & 0xff;
                // 후보 자신이 패딩 명령어이면 함수 시작이 아니다 (진짜 시작은 패딩이 끝난 다음이다)
                if (paddingLength(mem, a) == 0 && fm.getFunctionContaining(a) == null && looksLikePrologue(mem, a)) {
                    if (afterPadding(mem, a)) {
                        addCandidate(a, SRC_PROLOGUE);
                    } else if (afterTerminator(mem, a) && cur != 0xc3 && cur != 0xc2 && cur != 0xe9 && cur != 0xb8
                            && fm.getFunctionContaining(a.subtract(1)) == null) {
                        // 앞 명령이 속한 함수도 없는 경우만 (함수 안의 ret 직후는 제외). 너무 짧은 시작 바이트는 제외해 오탐을 줄인다
                        addCandidate(a, SRC_AFTER_RET);
                    }
                }
                a = a.next();
            }
        }

        // 1b) 기존 함수의 끝 바로 다음 주소: 패딩 없이 이어지는 함수(예: FUN_004b1e80)는 CC 규칙으로 못 찾는다
        for (Function f : fm.getFunctions(true)) {
            if (monitor.isCancelled()) return;
            Address next = f.getBody().getMaxAddress().next();
            if (next == null) continue;
            // 함수 끝 다음에 패딩이 있으면 건너뛴다 (패딩 뒤에서 시작하는 함수는 규칙 1 이 다루므로 여기서는 제외)
            if (paddingLength(mem, next) != 0) continue;
            if (fm.getFunctionContaining(next) != null) continue;
            if (inCodeBlocks(codeBlocks, next) && looksLikePrologue(mem, next)) {
                addCandidate(next, SRC_ADJACENT);
            }
        }

        // 2) 데이터 구간의 코드 포인터: 4바이트 정렬 값이 코드 구간 안이고 함수에 속하지 않으면 후보
        pointerCounts.clear();
        for (MemoryBlock blk : mem.getBlocks()) {
            if (blk.isExecute() || !blk.isInitialized()) continue;
            Address a = blk.getStart();
            Address end = blk.getEnd().subtract(3);
            while (a.compareTo(end) < 0) {
                if (monitor.isCancelled()) return;
                Address target = toAddr(mem.getInt(a) & 0xffffffffL);
                if (inCodeBlocks(codeBlocks, target) && fm.getFunctionContaining(target) == null
                        && fm.getFunctionAt(target) == null) {
                    // 명령어 중간을 가리키는 우연한 값을 줄이기 위해 앞 바이트 패딩(CC)/ret 와 프롤로그 모양을 요구한다
                    // 블록 맨 앞 주소 등 앞 바이트가 없는 경우는 읽기 예외가 나므로 후보에서 제외한다
                    boolean afterPad = afterPadding(mem, target);
                    try {
                        afterPad = afterPad || (mem.getByte(target.subtract(1)) & 0xff) == 0xc3;
                    } catch (MemoryAccessException ex) {
                        // 앞 바이트가 없는 위치는 위 패딩 판별 결과만 쓴다
                    }
                    if (afterPad && looksLikePrologue(mem, target)) {
                        addCandidate(target, SRC_POINTER);
                        pointerCounts.merge(target.getOffset(), 1, Integer::sum);
                    }
                }
                a = a.add(4);
            }
        }
    }

    // 스크립트 진입점: 인자는 결과 C 파일("-" 이면 디컴파일하지 않음), 후보 목록 TSV 파일,
    // (선택) 함수 밖 코드 구간 TSV 파일, (선택) "commit" 순서.
    // "commit" 을 주면 라운드마다 Ghidra 자동 분석을 다시 돌려 새 함수에 스택·참조 분석을 적용한다
    // (프로젝트를 -readOnly 없이 열어야 저장된다).
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2) {
            printerr("사용법: RecoverMissing.java <결과 C 파일|-> <후보 목록 TSV 파일> [<함수 밖 코드 구간 TSV 파일>] [commit]");
            return;
        }
        boolean commit = args.length >= 4 && args[3].equals("commit");
        boolean writeC = !args[0].equals("-");
        Memory mem = currentProgram.getMemory();
        FunctionManager fm = currentProgram.getFunctionManager();

        // 코드 구간(실행 가능 블록)의 주소 범위를 모은다
        List<MemoryBlock> codeBlocks = new ArrayList<>();
        for (MemoryBlock blk : mem.getBlocks()) {
            if (blk.isExecute() && blk.isInitialized()) {
                codeBlocks.add(blk);
            }
        }

        // 후보 목록 전체(모든 라운드 합계)와 결과 C 파일을 라운드 사이에 유지한다
        Map<Long, Integer> allCandidates = new TreeMap<>();
        Map<Long, Integer> allPointerCounts = new TreeMap<>();
        DecompInterface decomp = new DecompInterface();
        decomp.openProgram(currentProgram);
        int ok = 0, fail = 0, skipped = 0, noFunc = 0, retried = 0, createdTotal = 0;
        try (PrintWriter out = new PrintWriter(new OutputStreamWriter(
                writeC ? new FileOutputStream(args[0]) : OutputStream.nullOutputStream(), StandardCharsets.UTF_8))) {
            // 새 함수를 만들면 그 뒤에 이어진 코드가 새 후보가 되므로 더 나오지 않을 때까지 반복한다
            for (int round = 1; round <= MAX_ROUNDS; round++) {
                candidates.clear();
                collectCandidates(mem, fm, codeBlocks);
                if (monitor.isCancelled()) break;
                println("라운드 " + round + ": 후보 " + candidates.size() + "개 수집");
                if (candidates.isEmpty()) break;
                for (Map.Entry<Long, Integer> e : candidates.entrySet()) {
                    allCandidates.merge(e.getKey(), e.getValue(), (x, y) -> x | y);
                }
                allPointerCounts.putAll(pointerCounts);
                int created = 0;
                // 후보마다 함수를 만들어 디컴파일한다 (앞서 만든 함수가 뒤 후보를 덮으면 건너뜀)
                for (Map.Entry<Long, Integer> e : candidates.entrySet()) {
                    if (monitor.isCancelled()) break;
                    Address entry = toAddr(e.getKey());
                    if (fm.getFunctionContaining(entry) != null && fm.getFunctionAt(entry) == null) {
                        skipped++;
                        continue;
                    }
                    Function f = fm.getFunctionAt(entry);
                    if (f == null) {
                        disassemble(entry);
                        f = createFunction(entry, null);
                    }
                    if (f == null) {
                        // 기존 데이터·명령어 정의와 겹쳐 실패했을 수 있으므로 그 자리를 지우고 다시 시도한다
                        // (다음 함수의 시작을 넘어 지우지 않도록 범위를 제한한다)
                        Address limit = entry.add(0xff);
                        for (Function nextFn : fm.getFunctions(entry.next(), true)) {
                            if (nextFn.getEntryPoint().compareTo(limit) <= 0) {
                                limit = nextFn.getEntryPoint().previous();
                            }
                            break;
                        }
                        clearListing(entry, limit);
                        disassemble(entry);
                        f = createFunction(entry, null);
                        if (f != null) retried++;
                    }
                    if (f == null) {
                        out.println("// ==== " + Long.toHexString(e.getKey()) + " 함수를 만들 수 없음");
                        noFunc++;
                        continue;
                    }
                    created++;
                    createdTotal++;
                    if (!writeC) continue;
                    out.println("// ==== " + f.getName() + " @ " + f.getEntryPoint() + " [출처: "
                            + sourceName(e.getValue()) + ", 포인터 " + pointerCounts.getOrDefault(e.getKey(), 0) + "회]");
                    DecompileResults r = decomp.decompileFunction(f, DECOMPILE_TIMEOUT_SEC, monitor);
                    if (r.decompileCompleted()) {
                        out.println(r.getDecompiledFunction().getC());
                        ok++;
                    } else {
                        out.println("// 디컴파일 실패: " + r.getErrorMessage());
                        fail++;
                    }
                }
                println("라운드 " + round + ": 함수 " + created + "개 생성");
                if (created == 0) break;
                // 확정 모드: 새로 만든 함수에 Ghidra 분석기(스택, 호출 규약, 참조, switch)를 적용한다
                if (commit) analyzeChanges(currentProgram);
            }
        } finally {
            decomp.dispose();
        }

        // 후보 목록을 TSV 로 기록한다 (주소, 출처, 포인터 수)
        try (PrintWriter tsv = new PrintWriter(new OutputStreamWriter(
                new FileOutputStream(args[1]), StandardCharsets.UTF_8))) {
            tsv.println("address\tsource\tpointer_refs");
            for (Map.Entry<Long, Integer> e : allCandidates.entrySet()) {
                tsv.println(Long.toHexString(e.getKey()) + "\t" + sourceName(e.getValue()) + "\t"
                        + allPointerCounts.getOrDefault(e.getKey(), 0));
            }
        }

        // 복구 뒤에도 어떤 함수에도 속하지 않는 코드 구간(INT3 패딩 제외, 16바이트 이상)을 TSV 로 남긴다
        if (args.length >= 3) {
            int gapRanges = 0;
            long gapBytes = 0;
            try (PrintWriter gaps = new PrintWriter(new OutputStreamWriter(
                    new FileOutputStream(args[2]), StandardCharsets.UTF_8))) {
                gaps.println("start\tlength\tfirst_bytes");
                for (MemoryBlock blk : codeBlocks) {
                    Address a = blk.getStart();
                    Address blockEnd = blk.getEnd();
                    while (a != null && a.compareTo(blockEnd) <= 0) {
                        if (monitor.isCancelled()) break;
                        // 함수에 속하지 않고 패딩(CC)도 아닌 바이트가 시작되면 함수가 다시 나올 때까지 길이를 잰다
                        if (fm.getFunctionContaining(a) == null && (mem.getByte(a) & 0xff) != 0xcc) {
                            Address s = a;
                            Address e2 = a;
                            while (e2 != null && e2.compareTo(blockEnd) <= 0 && fm.getFunctionContaining(e2) == null) {
                                e2 = e2.next();
                            }
                            // 끝쪽 CC 패딩을 제외한 실제 길이
                            Address last = (e2 == null) ? blockEnd : e2.previous();
                            while (last.compareTo(s) > 0 && (mem.getByte(last) & 0xff) == 0xcc) last = last.previous();
                            long len = last.getOffset() - s.getOffset() + 1;
                            if (len >= 16) {
                                StringBuilder hex = new StringBuilder();
                                for (int i = 0; i < 8 && i < len; i++) {
                                    hex.append(String.format("%02x", mem.getByte(s.add(i)) & 0xff));
                                }
                                gaps.println(Long.toHexString(s.getOffset()) + "\t" + len + "\t" + hex);
                                gapRanges++;
                                gapBytes += len;
                            }
                            a = e2;
                        } else {
                            a = a.next();
                        }
                    }
                }
            }
            println("함수 밖 코드 구간(16바이트 이상): " + gapRanges + "곳, 합계 " + gapBytes + "바이트");
        }
        println("누락 함수 복구 완료: 함수 생성 " + createdTotal + ", 디컴파일 성공 " + ok + ", 실패 " + fail + ", 함수 생성 불가 " + noFunc + ", 건너뜀 " + skipped
                + ", 지우고 재시도로 생성 " + retried);
    }
}

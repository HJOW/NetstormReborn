// 검토된 5개 함수만 자료형·이름·ECX/스택 원형을 적용하여 별도 디컴파일로 내보낸다.
// run_script.ps1 -Refined의 기본 읽기 전용 모드로 사용한다. 프로젝트 변경은 저장하지 않는다.
// 전체 Parameter ID 추정과 달리 원형·오프셋은 두 판본 x86 차등 검증에서 확인한 것만 사용한다.
// 사용: ApplyRecoveredTypes.java <출력 C 파일>
//@category NetStorm
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.SourceType;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.*;

public class ApplyRecoveredTypes extends GhidraScript {
    // 원본 SHA-256. 주소 원형을 다른 빌드에 잘못 적용하지 않는다.
    private static final String PATCH_SHA = "a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236";
    // 참고 CD판의 동일 주소 배치를 가진 바이너리 SHA-256.
    private static final String CD_SHA = "613500a394b3062e914604eb7d09896d678192be24545d0e575082a87c851233";
    // 프레임 검색 3개·설정 키 검색·원시 값 읽기의 검토된 패치판 주소.
    private static final long[] PATCH_ENTRIES = {0x49a9a0,0x49a9e0,0x49aa30,0x43fd70,0x440150};
    // 동일 의미인 CD판 주소. 다른 오버로드 00444350을 마스크 검색으로 지정하지 않는다.
    private static final long[] CD_ENTRIES = {0x444350,0x444410,0x444460,0x42a270,0x42ab70};
    // 복원된 의미 이름. 원본 심볼이 복구됐다는 뜻이 아니다.
    private static final String[] NAMES = {"FrameFindNumber","FrameFindMasked","FrameFindFlags","ConfigFindKey","ConfigGetRaw"};
    // 실제 ret N의 스택 복구 크기. 두 판본에서 따로 검사한다.
    private static final int[] PURGES = {12,16,12,0,8};

    // 원본 32비트 포인터를 명시적으로 만든다. 호스트 Java의 포인터 크기는 사용하지 않는다.
    private DataType pointer(DataType type) { return new PointerDataType(type, 4, currentProgram.getDataTypeManager()); }
    // 원본 진입 시 ESP+offset에 놓이는 스택 인자를 만든다.
    private Parameter stack(String name, DataType type, int offset) throws Exception {
        return new ParameterImpl(name, type, offset, currentProgram);
    }
    // this는 실제 ECX 레지스터 입력에 배정한다. 자동 this 추가로 인자가 중복되는 것을 막는다.
    private Parameter object(DataType type) throws Exception {
        return new ParameterImpl("object", pointer(type), currentProgram.getRegister("ECX"), currentProgram);
    }
    // 검토된 함수에서 실제 ret N을 다시 읽어 원형 적용 전 확인한다.
    private void checkRet(Function function, int purge) {
        boolean seen = false;
        InstructionIterator instructions = currentProgram.getListing().getInstructions(function.getBody(), true);
        // 몸체 안의 모든 return이 같은 인자 크기로 복구되는지 확인한다.
        while (instructions.hasNext()) {
            Instruction instruction = instructions.next();
            if (!instruction.getMnemonicString().startsWith("RET")) continue;
            seen = true;
            int actual = instruction.getNumOperands() == 0 ? 0 : (int)instruction.getScalar(0).getUnsignedValue();
            if (actual != purge) throw new IllegalStateException("ret N 불일치: " + function.getName());
        }
        if (!seen) throw new IllegalStateException("return 없음: " + function.getName());
    }

    // 구조체의 확인된 필드와 검토된 원형만 적용하고 같은 프로세스에서 바로 내보낸다.
    @Override public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 1) throw new IllegalArgumentException("ApplyRecoveredTypes.java <출력 C 파일>");
        String sha = currentProgram.getExecutableSHA256();
        boolean patch = PATCH_SHA.equalsIgnoreCase(sha);
        if (!patch && !CD_SHA.equalsIgnoreCase(sha)) throw new IllegalArgumentException("검토하지 않은 바이너리 SHA-256");
        if (currentProgram.getDefaultPointerSize() != 4) throw new IllegalArgumentException("원본 x86 32비트 전용");
        DataTypeManager manager = currentProgram.getDataTypeManager();
        CategoryPath category = new CategoryPath("/NetstormRecovered");
        StructureDataType code = new StructureDataType(category, "FrameCode32", 4, manager);
        code.replaceAtOffset(0, ByteDataType.dataType, 1, "side", "방향");
        code.replaceAtOffset(1, ByteDataType.dataType, 1, "variant", "변형");
        code.replaceAtOffset(2, ByteDataType.dataType, 1, "number", "번호");
        code.replaceAtOffset(3, SignedByteDataType.dataType, 1, "flags", "원본 movsx의 부호 확장");
        DataType codeType = manager.addDataType(code, DataTypeConflictHandler.REPLACE_HANDLER);
        // 후처리 0049b0d0 / CD 00444e10의 인덱스 계산에서 판본별 stride 500/468을 확인했다.
        int typeSize = patch ? 500 : 468;
        StructureDataType type = new StructureDataType(category, "RiftTypeFrameView32", typeSize, manager);
        type.replaceAtOffset(0x114, IntegerDataType.dataType, 4, "frameCount", "코드 배열의 개수");
        type.replaceAtOffset(0x124, pointer(codeType), 4, "frameCodes", "4바이트 원소 배열");
        DataType typeView = manager.addDataType(type, DataTypeConflictHandler.REPLACE_HANDLER);
        StructureDataType config = new StructureDataType(category, "ConfigTextPrefix32", 4, manager);
        config.replaceAtOffset(0, pointer(CharDataType.dataType), 4, "text", "확인된 첫 필드만 표현");
        DataType configView = manager.addDataType(config, DataTypeConflictHandler.REPLACE_HANDLER);
        long[] entries = patch ? PATCH_ENTRIES : CD_ENTRIES;
        List<Function> functions = new ArrayList<>();
        // 검토된 다섯 함수 이외에는 인자·반환형을 잠그지 않는다.
        for (int i = 0; i < entries.length; ++i) {
            Function function = getFunctionAt(toAddr(entries[i]));
            if (function == null) throw new IllegalStateException("정밀 함수 없음: " + Long.toHexString(entries[i]));
            checkRet(function, PURGES[i]);
            List<Variable> parameters = new ArrayList<>();
            DataType result = IntegerDataType.dataType;
            if (i <= 2) {
                parameters.add(object(typeView));
                parameters.add(stack("side", ByteDataType.dataType, 4));
                parameters.add(stack("variant", ByteDataType.dataType, 8));
                parameters.add(stack(i == 2 ? "flags" : "number", i == 2 ? IntegerDataType.dataType : ByteDataType.dataType, 12));
                if (i == 1) parameters.add(stack("mask", UnsignedIntegerDataType.dataType, 16));
            } else if (i == 3) {
                result = pointer(CharDataType.dataType);
                parameters.add(stack("key", pointer(CharDataType.dataType), 4));
                parameters.add(stack("text", pointer(CharDataType.dataType), 8));
            } else {
                parameters.add(object(configView));
                parameters.add(stack("key", pointer(CharDataType.dataType), 4));
                parameters.add(stack("output", pointer(CharDataType.dataType), 8));
            }
            function.setName(NAMES[i], SourceType.USER_DEFINED);
            Variable returns = new ReturnParameterImpl(result, currentProgram.getRegister("EAX"), currentProgram);
            function.updateFunction(i == 3 ? "__cdecl" : "__thiscall", returns, parameters,
                Function.FunctionUpdateType.CUSTOM_STORAGE, true, SourceType.USER_DEFINED);
            function.setStackPurgeSize(PURGES[i]);
            functions.add(function);
        }
        DecompInterface decomp = new DecompInterface();
        decomp.openProgram(currentProgram);
        try (PrintWriter out = new PrintWriter(new OutputStreamWriter(new FileOutputStream(args[0]), StandardCharsets.UTF_8))) {
            out.println("// 검토한 함수의 자료형 복원. 원본 SHA-256: " + sha);
            // 구조체 필드 이름으로 읽히는 디컴파일을 별도 파일에 기록한다.
            for (Function function : functions) {
                DecompileResults result = decomp.decompileFunction(function, 60, monitor);
                if (!result.decompileCompleted()) throw new IllegalStateException(result.getErrorMessage());
                out.println("// ==== " + function.getName() + " @ " + function.getEntryPoint());
                out.println(result.getDecompiledFunction().getC());
            }
            out.println("// recovered-complete:5");
        } finally { decomp.dispose(); }
        println("자료형 적용 디컴파일 완료: " + functions.size() + "개");
    }
}

// 다리 검증에서 확인한 원형·구조체 필드를 읽기 전용 프로젝트에 적용하여 별도 C로 내보낸다.
// 전체 Parameter ID를 확정하지 않는다. 수명 함수는 접두 구간만 기계어 검증했다.
//@category NetStorm
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.SourceType;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.*;

public class ApplyBridgeTypes extends GhidraScript {
    // 검토한 바이너리 해시와 정확한 주소 배치를 제한한다.
    private static final String PATCH_SHA = "a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236";
    // CD판은 자료형 크기와 Squid 프레임 필드가 다르다.
    private static final String CD_SHA = "613500a394b3062e914604eb7d09896d678192be24545d0e575082a87c851233";
    // 추첨·난수·생성·반복·셀·프레임·한 방향 검사·수명 함수와 패치판 표면 조회.
    private static final long[] PATCH = {0x4257c0,0x4558c0,0x425c20,0x425860,0x425700,0x49a940,0x421770,0x421c30,0x40eaf0};
    // CD판 표면 조회는 검사 보조 함수에 인라인되어 있다.
    private static final long[] CD = {0x41fc70,0x48cbd0,0x41fcf0,0x41feb0,0x41fbb0,0x4442f0,0x449e30,0x44a1c0};
    // 이 이름들은 복원한 의미 이름이며 원본 심볼은 아니다.
    private static final String[] NAMES = {"BridgeDrawPattern","GameRandomBounded","CanonConstruct","CanonAdvance","CanonCell",
        "CanonFindFrame","BridgeIsOpen","BridgeReduceLife","SurfaceAtRoundedPosition"};
    // 실제 ret N으로 확인한 스택 인자 크기다.
    private static final int[] PURGES = {0,0,24,0,20,12,8,4,0};

    // Ghidra 호스트와 무관하게 원본 포인터는 4바이트다.
    private DataType pointer(DataType type) { return new PointerDataType(type,4,currentProgram.getDataTypeManager()); }
    // 원본 ECX this 입력을 직접 배정하여 자동 this 중복을 피한다.
    private Parameter object(DataType type) throws Exception {
        return new ParameterImpl("object",pointer(type),currentProgram.getRegister("ECX"),currentProgram);
    }
    // 진입 ESP+offset의 스택 인자를 지정한다.
    private Parameter stack(String name,DataType type,int offset) throws Exception {
        return new ParameterImpl(name,type,offset,currentProgram);
    }
    // 함수 몸체의 모든 ret 크기가 검토 값과 같은지 재확인한다.
    private void checkRet(Function function,int purge) {
        boolean seen=false;
        InstructionIterator instructions=currentProgram.getListing().getInstructions(function.getBody(),true);
        // 여러 반환 분기도 모두 같은 크기여야 한다.
        while (instructions.hasNext()) {
            Instruction instruction=instructions.next();
            if (!instruction.getMnemonicString().equalsIgnoreCase("RET")) continue;
            seen=true;
            int actual=instruction.getNumOperands()==0 ? 0 : (int)instruction.getScalar(0).getUnsignedValue();
            if (actual!=purge) throw new IllegalStateException("ret 불일치: "+function.getEntryPoint());
        }
        if (!seen) throw new IllegalStateException("ret 없음: "+function.getEntryPoint());
    }
    // 확인된 필드를 원본 오프셋에 지정한다. 미확정 구간은 undefined로 남긴다.
    private void field(StructureDataType structure,int offset,DataType type,String name) {
        structure.replaceAtOffset(offset,type,type.getLength(),name,"원본 기계어의 읽기/쓰기 오프셋");
    }

    // 타입을 적용해 호출자까지 다시 계산한 뒤 별도 파일만 기록한다.
    @Override public void run() throws Exception {
        String[] args=getScriptArgs();
        if (args.length!=1) throw new IllegalArgumentException("ApplyBridgeTypes.java <출력 C 파일>");
        String sha=currentProgram.getExecutableSHA256();
        boolean patch=PATCH_SHA.equalsIgnoreCase(sha);
        if (!patch && !CD_SHA.equalsIgnoreCase(sha)) throw new IllegalArgumentException("검토하지 않은 바이너리");
        if (currentProgram.getDefaultPointerSize()!=4) throw new IllegalArgumentException("x86 32비트 전용");
        DataTypeManager manager=currentProgram.getDataTypeManager();
        CategoryPath category=new CategoryPath("/NetstormBridgeRecovered");
        DataType integer=IntegerDataType.dataType, uinteger=UnsignedIntegerDataType.dataType, floating=FloatDataType.dataType;
        StructureDataType frame=new StructureDataType(category,"FrameCode32",4,manager);
        field(frame,0,ByteDataType.dataType,"side"); field(frame,1,ByteDataType.dataType,"variant");
        field(frame,2,ByteDataType.dataType,"number"); field(frame,3,ByteDataType.dataType,"flags");
        StructureDataType type=new StructureDataType(category,"TypeFrameView32",patch?500:468,manager);
        field(type,0x114,integer,"frameCount"); field(type,0x118,integer,"defaultFrame"); field(type,0x124,pointer(frame),"frameCodes");
        StructureDataType cell=new StructureDataType(category,"CanonCell32",4,manager);
        field(cell,0,ByteDataType.dataType,"variationChar"); field(cell,1,ByteDataType.dataType,"side");
        field(cell,3,ByteDataType.dataType,"labelChar");
        StructureDataType pattern=new StructureDataType(category,"CanonPattern32",72,manager);
        field(pattern,0,integer,"weight"); field(pattern,4,integer,"width"); field(pattern,8,integer,"height");
        field(pattern,12,new ArrayDataType(cell,15,4),"cells");
        StructureDataType decoder=new StructureDataType(category,"CanonDecoder32",0x54,manager);
        String[] integers={"typeId","shape","rotation","mirrored","frame","valid","label","step"};
        // 첫 8개 int는 +0~+1c다.
        for (int i=0;i<integers.length;++i) field(decoder,i*4,integer,integers[i]);
        field(decoder,0x20,floating,"x"); field(decoder,0x24,floating,"y"); field(decoder,0x28,pointer(type),"type");
        field(decoder,0x2c,floating,"originX"); field(decoder,0x30,floating,"originY"); field(decoder,0x34,pointer(pattern),"pattern");
        String[] counters={"outerCount","innerCount","patternX","patternY","outer","inner","explicitFrame"};
        // +38부터의 순회 필드다.
        for (int i=0;i<counters.length;++i) field(decoder,0x38+i*4,integer,counters[i]);
        StructureDataType squid=new StructureDataType(category,"SquidBridgeView32",patch?50:36,manager);
        field(squid,0,pointer(pointer(VoidDataType.dataType)),"vtable"); field(squid,10,ByteDataType.dataType,"typeId");
        field(squid,0xc,UnsignedShortDataType.dataType,"lifeAndFlags"); field(squid,0xe,floating,"x"); field(squid,0x12,floating,"y");
        field(squid,patch?0x24:0x22,patch?integer:ByteDataType.dataType,"frame");
        StructureDataType list=new StructureDataType(category,"SurfaceIdList32",12,manager);
        field(list,0,pointer(integer),"items"); field(list,4,integer,"capacity"); field(list,8,integer,"count");
        long[] entries=patch?PATCH:CD;
        List<Function> functions=new ArrayList<>();
        // 검토된 함수만 원형을 잠근다. 미검토 가상 함수나 전체 프로그램은 건드리지 않는다.
        for (int i=0;i<entries.length;++i) {
            Function function=getFunctionAt(toAddr(entries[i]));
            if (function==null) throw new IllegalStateException("함수 없음: "+Long.toHexString(entries[i]));
            checkRet(function,PURGES[i]);
            List<Variable> parameters=new ArrayList<>();
            DataType result=integer;
            boolean cdecl=i<=1 || i==8;
            if (i<=1) parameters.add(stack(i==0?"random":"limit",i==0?integer:uinteger,4));
            else if (i==2) {
                parameters.add(object(decoder)); result=pointer(decoder);
                parameters.add(stack("typeId",integer,4)); parameters.add(stack("shape",integer,8)); parameters.add(stack("direction",integer,12));
                parameters.add(stack("x",floating,16)); parameters.add(stack("y",floating,20)); parameters.add(stack("explicitFrame",integer,24));
            } else if (i==3) { parameters.add(object(decoder)); result=VoidDataType.dataType; }
            else if (i==4) {
                parameters.add(object(pattern)); parameters.add(stack("x",integer,4)); parameters.add(stack("y",integer,8));
                parameters.add(stack("side",pointer(integer),12)); parameters.add(stack("number",pointer(integer),16)); parameters.add(stack("label",pointer(integer),20));
            } else if (i==5) {
                parameters.add(object(type)); parameters.add(stack("side",CharDataType.dataType,4));
                parameters.add(stack("variant",CharDataType.dataType,8)); parameters.add(stack("number",CharDataType.dataType,12));
            } else if (i==6) {
                parameters.add(object(squid)); parameters.add(stack("neighbors",pointer(list),4)); parameters.add(stack("direction",integer,8));
            } else if (i==7) {
                parameters.add(object(squid)); parameters.add(stack("reduction",integer,4)); result=VoidDataType.dataType;
            } else {
                parameters.add(stack("x",floating,4)); parameters.add(stack("y",floating,8)); result=uinteger;
            }
            function.setName(NAMES[i],SourceType.USER_DEFINED);
            Variable returns=result==VoidDataType.dataType ? new ReturnParameterImpl(result,currentProgram)
                : new ReturnParameterImpl(result,currentProgram.getRegister("EAX"),currentProgram);
            function.updateFunction(cdecl?"__cdecl":"__thiscall",returns,parameters,
                Function.FunctionUpdateType.CUSTOM_STORAGE,true,SourceType.USER_DEFINED);
            function.setStackPurgeSize(PURGES[i]); functions.add(function);
        }
        DecompInterface decomp=new DecompInterface(); decomp.openProgram(currentProgram);
        try (PrintWriter out=new PrintWriter(new OutputStreamWriter(new FileOutputStream(args[0]),StandardCharsets.UTF_8))) {
            out.println("// 검토한 다리 자료형. 원본 SHA-256: "+sha);
            out.println("// BridgeReduceLife는 외부 효과 이전 접두 구간만 x86 대조했다.");
            // 구조체·스택 인자가 반영된 C를 각 함수마다 기록한다.
            for (Function function:functions) {
                DecompileResults result=decomp.decompileFunction(function,60,monitor);
                if (!result.decompileCompleted()) throw new IllegalStateException(result.getErrorMessage());
                out.println("// ==== "+function.getName()+" @ "+function.getEntryPoint());
                out.println(result.getDecompiledFunction().getC());
            }
            out.println("// bridge-types-complete:"+functions.size());
        } finally { decomp.dispose(); }
        println("다리 자료형 디컴파일 완료: "+functions.size()+"개");
    }
}

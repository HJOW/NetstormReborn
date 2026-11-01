using static Netstorm.Core.Original.RawStorageBytes;

namespace Netstorm.Core.Original;

/// <summary>진단 뒤 계속하는 원본 위치를 구별한다. 물리 범위 초과는 이 진단과 별도로 거부한다.</summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public enum DataManagerImageAssertion
{
    /// <summary>MarkStream의 시작 pos가 2가 아니다.</summary>
    SourcePosition,

    /// <summary>이름이 남았는데 mark가 비어 있다(-1).</summary>
    MissingMark,

    /// <summary>이전 mark가 가리키는 길이와 현재 pos-mark가 다르다.</summary>
    MarkLength,

    /// <summary>mark를 쓰는 위치(pos)가 end를 넘었다.</summary>
    MarkPosition,

    /// <summary>이름표 첫 구분자가 '/'가 아니다.</summary>
    NameSeparator,
}

/// <summary>SNART 이름표(manager 가상 +4가 돌려주는 같은 변경 가능 표)를 빌리는 장치다.</summary>
/// <remarks>2026-11-01 0.128에서 추가했다. 표 포인터는 호출마다 같은 물리 표여야 한다.</remarks>
public delegate Span<byte> SnartNamesLookup();

/// <summary>
/// 파일 이미지 읽기가 쓰는 외부 장치 연결이다(원본 <c>DataManagerImageHooks</c>).
/// 가상 이름표와 같은 노드·버퍼의 실제 경계를 제공한다.
/// </summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public sealed class DataManagerImageHooks
{
    /// <summary>생성·검색·제거·압축은 같은 실제 노드와 버퍼를 빌린다.</summary>
    public DataManagerResizeHooks Streams { get; set; } = new();

    /// <summary>manager 가상 +4가 돌려준 같은 변경 가능 SNART 이름표다.</summary>
    public SnartNamesLookup? Names { get; set; }

    /// <summary>해당 원본 진단이 선택된 위치에서만 호출한다.</summary>
    public Action<DataManagerImageAssertion>? Assertion { get; set; }
}

/// <summary>Template manager +c의 실제 서명 판정에 필요한 현재 메모리·전역·파일 경계다(원본 <c>TemplateFileSignatureHooks</c>).</summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public sealed class TemplateFileSignatureHooks
{
    /// <summary>같은 입력 포인터의 물리 버퍼를 빌린다. 읽기 조회 자체는 상태를 변경하지 않는다.</summary>
    public RawBytesLookup? Buffer { get; set; }

    /// <summary>현재 signed numTypes다. 원본의 numTypes&lt;256 비교에 쓴다.</summary>
    public Func<int>? TypeCount { get; set; }

    /// <summary>원본 진단 뒤 복귀할 수 있으며 이후 현재 source를 다시 읽는다.</summary>
    public Action? TooManyTypes { get; set; }

    /// <summary>서명 거부 때 가상 +0에서 다시 읽는 현재 파일 이름이다.</summary>
    public Func<string>? FileName { get; set; }

    /// <summary>원본 41aef0의 파일 제거 요청 경계다. 보호 자료의 실제 삭제 허가는 이 함수가 부여하지 않는다.</summary>
    public Action<string>? RejectFile { get; set; }
}

/// <summary>실제 파일 읽기 호출이다. 이름·1·길이 DWORD 주소·0을 받고 읽은 data 토큰을 돌려준다(원본 41baa0).</summary>
/// <param name="name">현재 파일 이름.</param>
/// <param name="mode">원본 인자 1.</param>
/// <param name="length">읽은 길이를 받을 DWORD 4바이트(source +12).</param>
/// <param name="flags">원본 인자 0.</param>
/// <remarks>2026-11-01 0.128에서 추가했다. 길이 슬롯을 Span으로 받으므로 전용 대리자를 쓴다.</remarks>
public delegate uint DataManagerFileReader(string name, int mode, Span<byte> length, int flags);

/// <summary>현재 같은 MarkStream20을 받는 manager 가상 +c 서명 판정이다.</summary>
/// <param name="source">MarkStream 20바이트.</param>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public delegate bool DataManagerSignatureReader(Span<byte> source);

/// <summary>원본 파일 할당과 가상 판정을 같은 MarkStream20·섹션 읽기에 연결한다(원본 <c>DataManagerFileHooks</c>).</summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public sealed class DataManagerFileHooks
{
    /// <summary>서명 성공 뒤 현재 같은 source로 실행할 섹션 본문의 장치다.</summary>
    public DataManagerImageHooks Image { get; set; } = new();

    /// <summary>읽기 요청 직전 가상 +0의 현재 이름이다.</summary>
    public Func<string>? FileName { get; set; }

    /// <summary>원본 41baa0이다.</summary>
    public DataManagerFileReader? ReadFile { get; set; }

    /// <summary>현재 같은 MarkStream20을 받는 manager 가상 +c다.</summary>
    public DataManagerSignatureReader? Signature { get; set; }

    /// <summary>읽기 성공·서명 실패·예외 뒤 현재 source.data 할당을 한 번만 반납한다.</summary>
    public Action<uint>? ReleaseFile { get; set; }
}

/// <summary>
/// 10.78 파일 이미지의 같은 manager/MarkStream20/이름표를 실제 섹션 목록에 읽는다: mark 진행(0044cdb0), 섹션 읽기(0044d380),
/// Template 파일 서명(004bce10), 파일 읽기 부모(0044d980)다. 큰 원본 부모의 인라인 본문도 같은 공통 하위와 효과 뒤 재조회 순서로 옮겼다.
/// cpppj <c>o/RawDataManagerImage</c>를 옮겼다.
/// </summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public static class RawDataManagerImage
{
    /// <summary>MarkStream 한 개의 실제 최소 물리 폭(MemStream 16 + mark DWORD 4)이다.</summary>
    private const int MarkStreamBytes = 20;

    /// <summary>mark를 저장하는 MarkStream 안의 위치다.</summary>
    private const int MarkOffset = 16;

    /// <summary>DataManager 한 개의 실제 최소 물리 폭이다.</summary>
    private const int ManagerBytes = 12;

    /// <summary>Template manager의 실제 물리 폭(276)이다.</summary>
    private const int TemplateManagerBytes = 0x114;

    /// <summary>섹션 노드 한 개의 실제 최소 물리 폭이다.</summary>
    private const int NodeBytes = 60;

    /// <summary>빈 mark를 나타내는 값(-1)이다.</summary>
    private const uint NoMark = 0xffffffffu;

    /// <summary>미연결 진단을 빈 성공으로 채우지 않는다.</summary>
    /// <param name="hooks">진단 장치.</param>
    /// <param name="kind">진단 종류.</param>
    private static void Assert(DataManagerImageHooks hooks, DataManagerImageAssertion kind)
    {
        Need(hooks.Assertion, "DataManager 파일 이미지의 실제 진단 미연결")(kind);
    }

    /// <summary>MarkStream은 첫 16바이트와 추가 mark DWORD가 모두 있어야 한다.</summary>
    /// <param name="source">검사할 MarkStream.</param>
    private static void Source(ReadOnlySpan<byte> source)
    {
        if (source.Length < MarkStreamBytes)
        {
            throw new ArgumentException("DataManager 파일 이미지의 실제 MarkStream20 누락");
        }
    }

    /// <summary>실제 접근할 범위만 빌린다. 길이 0은 포인터 조회를 생략한다.</summary>
    /// <param name="hooks">버퍼 조회 장치.</param>
    /// <param name="pointer">버퍼 핸들.</param>
    /// <param name="position">버퍼 안 시작 위치.</param>
    /// <param name="length">필요한 길이.</param>
    private static Span<byte> BufferRange(DataManagerImageHooks hooks, uint pointer, uint position, uint length)
    {
        if (length == 0)
        {
            return default;
        }
        if (pointer == 0 || hooks.Streams.Memory.Buffer is null)
        {
            throw new InvalidOperationException("DataManager 파일 이미지의 실제 버퍼 미연결");
        }
        Span<byte> bytes = hooks.Streams.Memory.Buffer(pointer);
        if (position > (uint)bytes.Length || length > (uint)bytes.Length - position)
        {
            throw OutOfRange("DataManager 파일 이미지의 실제 버퍼 물리 범위");
        }
        return bytes.Slice((int)position, (int)length);
    }

    /// <summary>원본 getMark는 부호 있는 16비트이며 길이 계산의 짧은 정수 wrap도 보존한다.</summary>
    /// <param name="source">MarkStream.</param>
    /// <param name="hooks">버퍼 조회 장치.</param>
    private static int Mark(ReadOnlySpan<byte> source, DataManagerImageHooks hooks)
    {
        Span<byte> bytes = BufferRange(hooks, Read(source, 0), Read(source, MarkOffset), 2);
        ushort value = (ushort)(bytes[0] | (bytes[1] << 8));
        return unchecked((short)value);
    }

    /// <summary>실제 60바이트 노드만 빌리고 잘못된 NULL을 임의의 새 노드로 대체하지 않는다.</summary>
    /// <param name="hooks">노드 조회 장치.</param>
    /// <param name="handle">노드 핸들. 0이면 거부한다.</param>
    private static Span<byte> Node(DataManagerImageHooks hooks, uint handle)
    {
        if (handle == 0 || hooks.Streams.Selection.Stream is null)
        {
            throw new InvalidOperationException("DataManager 파일 이미지의 실제 노드 미연결");
        }
        Span<byte> bytes = hooks.Streams.Selection.Stream(handle);
        if (bytes.Length < NodeBytes)
        {
            throw OutOfRange("DataManager 파일 이미지의 실제 노드60 누락");
        }
        return bytes;
    }

    /// <summary>004a8800/004a8850의 같은 SNART 표를 한 번 변환하고 지정 이름을 현재 길이로 읽는다.</summary>
    /// <param name="names">이름표.</param>
    /// <param name="index">이름 번호.</param>
    /// <param name="hooks">첫 구분자 진단 장치.</param>
    private static string Name(Span<byte> names, uint index, DataManagerImageHooks hooks)
    {
        return RawSnart.ReadName(names, index, () => Assert(hooks, DataManagerImageAssertion.NameSeparator));
    }

    /// <summary>
    /// 0044cdb0: 이전 mark의 길이를 확인하고 현재 pos를 mark로 기록한 뒤 signed pos/end를 진단하고 pos를 2 늘린다.
    /// 진단이 mark/pos/end를 바꾸면 원본 본문처럼 그 현재 값을 후속 쓰기에 쓴다.
    /// </summary>
    /// <param name="source">MarkStream 20바이트.</param>
    /// <param name="hooks">버퍼·진단 장치.</param>
    public static void AdvanceMark(Span<byte> source, DataManagerImageHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        Source(source);
        uint mark = Read(source, MarkOffset);
        if (mark != NoMark && Mark(source, hooks) != unchecked((int)(Read(source, 4) - mark)))
        {
            Assert(hooks, DataManagerImageAssertion.MarkLength);
        }
        Put(source, MarkOffset, Read(source, 4));
        if (unchecked((int)Read(source, 8)) < unchecked((int)Read(source, 4)))
        {
            Assert(hooks, DataManagerImageAssertion.MarkPosition);
        }
        Put(source, 4, Read(source, 4) + 2);
    }

    /// <summary>
    /// 0044d380: 이름표 순회→생성→복사→빈 섹션 유지→기존 제거→추가→mark 진행을 수행한다.
    /// 입력 pos는 payload 뒤에 갱신하고 빈 새 섹션이 기존 것을 덮지 않는 원본 병합 순서를 유지한다.
    /// </summary>
    /// <param name="manager">DataManager 12바이트.</param>
    /// <param name="source">MarkStream 20바이트.</param>
    /// <param name="hooks">노드·목록·버퍼·이름표·진단 장치.</param>
    public static void ReadImage(Span<byte> manager, Span<byte> source, DataManagerImageHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        Source(source);
        if (manager.Length < ManagerBytes || hooks.Names is null)
        {
            throw new ArgumentException("DataManager 파일 이미지의 manager/가상 이름 표 미연결");
        }
        Span<byte> names = hooks.Names();
        if (Read(source, 4) != 2)
        {
            Assert(hooks, DataManagerImageAssertion.SourcePosition);
        }
        AdvanceMark(source, hooks);
        // 마지막 빈 이름까지 원래 이름표 순서대로 읽는다. 마지막 mark는 end+2까지 진행할 수 있다
        for (uint index = 0; ; index++)
        {
            string name = Name(names, index, hooks);
            if (name.Length == 0)
            {
                return;
            }
            if (Read(source, MarkOffset) == NoMark)
            {
                Assert(hooks, DataManagerImageAssertion.MissingMark);
            }
            ushort narrow = unchecked((ushort)((uint)Mark(source, hooks) - 2));
            int signedLength = unchecked((short)narrow);
            uint length = unchecked((uint)signedLength);
            uint stream = Need(hooks.Streams.AllocateStream, "DataManager 파일 이미지의 실제 새 노드 할당 미연결")(NodeBytes);
            if (stream != 0)
            {
                RawDataManagerResize.ConstructStream(stream, name, signedLength, hooks.Streams);
            }
            Span<byte> target = Node(hooks, stream);
            uint position = Read(source, 4);
            uint pointer = Read(source, 0);
            uint end = position + length;
            Put(source, 4, end);
            if (unchecked((int)Read(source, 8)) < unchecked((int)end))
            {
                Put(source, 8, end);
            }
            RawMemStream.Write(target, BufferRange(hooks, pointer, position, length), length, hooks.Streams.Memory);
            Put(target, 4, 0);
            uint existing = RawDataManagerSelection.FindSection(manager, name, hooks.Streams.Selection);
            if (existing != 0 && (Read(target, 0) == 0 || Read(target, 8) == 0))
            {
                RawDataManagerLifetime.DestroyMemStream(target, hooks.Streams.List.Lifetime.ReleaseBuffer);
                Need(hooks.Streams.List.Lifetime.ReleaseStream, "DataManager 빈 새 섹션의 실제 노드 반납 미연결")(stream);
            }
            else
            {
                if (existing != 0)
                {
                    RawDataManagerList.Remove(manager, existing, hooks.Streams.List);
                }
                RawDataManagerList.Append(manager, stream, hooks.Streams.List);
            }
            AdvanceMark(source, hooks);
        }
    }

    /// <summary>
    /// 004bce10: 비영 data/end를 확인하고 현재 두 바이트를 읽으며 pos를 증가시키고 manager +10c/+110을 쓴 뒤 F 판정/거부 파일 요청을 한다.
    /// 서명은 논리 end의 두 바이트 길이를 추가로 검사하지 않는다. 원본은 data/end의 비영만 확인하며 물리 범위는 별도로 보호한다.
    /// </summary>
    /// <param name="manager">Template manager 276바이트.</param>
    /// <param name="source">MarkStream 20바이트.</param>
    /// <param name="hooks">타입 수·버퍼·파일 거부 장치.</param>
    /// <returns>서명이 'F'(0x46)이면 true.</returns>
    public static bool ReadTemplateFileSignature(Span<byte> manager, Span<byte> source, TemplateFileSignatureHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        Source(source);
        if (manager.Length < TemplateManagerBytes)
        {
            throw new ArgumentException("Template 파일 서명의 manager276 누락");
        }
        if (Read(source, 0) == 0 || Read(source, 8) == 0)
        {
            return false;
        }
        if (hooks.TypeCount is null || hooks.Buffer is null)
        {
            throw new InvalidOperationException("Template 파일 서명의 현재 타입 수/버퍼 미연결");
        }
        if (hooks.TypeCount() >= 256)
        {
            Need(hooks.TooManyTypes, "Template 파일 서명의 타입 수 진단 미연결")();
        }
        Put(manager, 0x10c, SignatureByte(source, hooks));
        byte flag = SignatureByte(source, hooks);
        Put(manager, 0x110, flag == 0xfe ? 1u : 0u);
        if (Read(manager, 0x10c) == 0x46)
        {
            return true;
        }
        if (hooks.FileName is null || hooks.RejectFile is null)
        {
            throw new InvalidOperationException("Template 잘못된 서명의 현재 파일 처리 미연결");
        }
        hooks.RejectFile(hooks.FileName());
        return false;
    }

    /// <summary>서명 바이트 한 개를 읽는다. 매 바이트마다 현재 data/pos를 다시 읽고 원본 DWORD 증가를 적용한다.</summary>
    /// <param name="source">MarkStream 20바이트.</param>
    /// <param name="hooks">버퍼 조회 장치.</param>
    private static byte SignatureByte(Span<byte> source, TemplateFileSignatureHooks hooks)
    {
        uint pointer = Read(source, 0);
        uint position = Read(source, 4);
        Span<byte> bytes = Need(hooks.Buffer, "Template 파일 서명의 현재 타입 수/버퍼 미연결")(pointer);
        if (pointer == 0 || position >= (uint)bytes.Length)
        {
            throw OutOfRange("Template 파일 서명의 실제 BYTE 물리 범위");
        }
        byte value = bytes[(int)position];
        Put(source, 4, position + 1u);
        return value;
    }

    /// <summary>
    /// 0044d980: MarkStream20 초기화→파일 읽기→end=alloc→가상 서명→섹션 본문→현재 파일 버퍼 반납이다.
    /// 원본 정상 복귀는 파일 할당만 반납한다. 호스트 예외에서도 같은 현재 포인터의 반납을 한 번만 시도한다.
    /// </summary>
    /// <param name="manager">DataManager 12바이트.</param>
    /// <param name="hooks">파일·서명·섹션 본문·반납 장치.</param>
    /// <returns>읽기와 서명이 성공하면 true.</returns>
    public static bool ReadFile(Span<byte> manager, DataManagerFileHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (manager.Length < ManagerBytes || hooks.FileName is null || hooks.ReadFile is null)
        {
            throw new ArgumentException("DataManager 파일 읽기 부모의 manager/파일 경계 누락");
        }
        // 원본의 첫 네 DWORD 0/mark -1이다
        byte[] source = new byte[MarkStreamBytes];
        Put(source, MarkOffset, NoMark);
        bool releaseAttempted = false;

        // 서명·본문이 source.data를 바꾸면 원본처럼 마지막의 현재 포인터를 반납한다. 반납 중 예외의 중복 해제는 막는다
        void Release()
        {
            if (releaseAttempted)
            {
                return;
            }
            releaseAttempted = true;
            uint pointer = Read(source, 0);
            if (pointer == 0)
            {
                return;
            }
            Need(hooks.ReleaseFile, "DataManager 실제 파일 할당 반납 미연결")(pointer);
        }

        try
        {
            uint pointer = hooks.ReadFile(hooks.FileName(), 1, source.AsSpan(12, 4), 0);
            Put(source, 0, pointer);
            Put(source, 8, Read(source, 12));
            if (pointer == 0)
            {
                return false;
            }
            DataManagerSignatureReader signature = Need(hooks.Signature, "DataManager 파일 읽기의 실제 가상 서명 미연결");
            if (!signature(source))
            {
                Release();
                return false;
            }
            ReadImage(manager, source, hooks.Image);
            Release();
            return true;
        }
        catch
        {
            Release();
            throw;
        }
    }
}

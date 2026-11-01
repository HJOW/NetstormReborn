namespace Netstorm.Core.Original;

/// <summary>0055b490가 현재 가리키는 NUL 쓰기 버퍼를 빌린다. 이름 값을 복제하지 않는다.</summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public delegate Span<byte> FortNameBufferLookup();

/// <summary>004410f0(고정 FileSpec 버퍼, 현재 name, 0, 0)의 NUL 포함 결과다. 다음 PathSpec 전까지 유효해야 한다.</summary>
/// <param name="name">현재 이름 NUL 포함 바이트.</param>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public delegate ReadOnlySpan<byte> FortPathSpec(ReadOnlySpan<byte> name);

/// <summary>004e65ed의 NUL 종료 대소문자 무시 비교다. 같으면 true를 돌려준다.</summary>
/// <param name="first">첫째 경로.</param>
/// <param name="second">둘째 경로.</param>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public delegate bool FortSamePath(ReadOnlySpan<byte> first, ReadOnlySpan<byte> second);

/// <summary>0041aef0의 이전 경로 삭제 요청이다. 이 모듈은 실제 파일을 조작하지 않는다.</summary>
/// <param name="path">삭제 요청 경로.</param>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public delegate void FortDeleteRequest(ReadOnlySpan<byte> path);

/// <summary>004414d0의 lastMultiplayerFort/lastEditFort 설정 기록과 그 시점 현재 이름이다.</summary>
/// <param name="key">설정 키.</param>
/// <param name="name">그 시점의 현재 이름.</param>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public delegate void FortConfigWrite(string key, ReadOnlySpan<byte> name);

/// <summary>
/// 원본 Client 전역은 현재 getter와 실제 dirty DWORD 참조를 통해 읽고 쓴다(원본 <c>FortCurrentNameGlobals</c>). getter는 부작용 없는 조회다.
/// </summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public sealed class FortCurrentNameGlobals
{
    /// <summary>00540cac의 현재 Template 슬롯이다. 본문이 재조회하는 시점마다 읽는다.</summary>
    public Func<uint>? Owner { get; set; }

    /// <summary>00594fb8의 현재 Fort 모드다. 제목 갱신 뒤 dirty 증가와 같은 순서로 읽는다.</summary>
    public Func<uint>? Fort { get; set; }

    /// <summary>005c85a4의 현재 Editor 모드다. 설정 callback 뒤에도 다시 읽는다.</summary>
    public Func<uint>? Editor { get; set; }

    /// <summary>00594fa0의 현재 Mission 모드다. 초기 Fort/Editor 관문에서만 읽는다.</summary>
    public Func<uint>? Mission { get; set; }

    /// <summary>00542678의 실제 공유 DWORD를 읽는다. 제목 갱신 뒤 원본처럼 한 번 증가한다.</summary>
    public Func<uint>? Dirty { get; set; }

    /// <summary>00542678의 실제 공유 DWORD에 쓴다.</summary>
    public Action<uint>? SetDirty { get; set; }
}

/// <summary>파일·장치·전체 Snapshot·Notify를 명시 경계로 두며 빈 기본 성공을 제공하지 않는다(원본 <c>FortCurrentNameHooks</c>).</summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public sealed class FortCurrentNameHooks
{
    /// <summary>고정 9슬롯의 실제 20바이트 헤더다. 진단 callback 뒤 같은 주소가 유효해야 한다.</summary>
    public RawBytesLookup? Header { get; set; }

    /// <summary>0055b490이 현재 가리키는 NUL 쓰기 버퍼다. 이름 값을 복제하지 않는다.</summary>
    public FortNameBufferLookup? CurrentName { get; set; }

    /// <summary>헤더의 실제 현재 manager 토큰 +c부터 256바이트다. 토큰 0/없는 주소는 명시 거부한다.</summary>
    public RawBytesLookup? ManagerName { get; set; }

    /// <summary>005327e0에 고정 FileSpec 버퍼 주소만 쓴다. 버퍼 내용을 fortSpec으로 덮지 않는다.</summary>
    public Action? SelectFileSpec { get; set; }

    /// <summary>004410f0(고정 FileSpec 버퍼, 현재 name, 0, 0)의 NUL 포함 결과다.</summary>
    public FortPathSpec? PathSpec { get; set; }

    /// <summary>39/88/Fortspec.cpp와 429/430/Template.cpp의 진단이다. 정상 반환하면 그대로 계속한다.</summary>
    public Action<uint>? Assertion { get; set; }

    /// <summary>004bfac0의 현재 Battle/Fort wrapper다. 현재 owner 선택은 실제 하위가 수행한다.</summary>
    public Action? Snapshot { get; set; }

    /// <summary>004bd360의 고정 this 슬롯 Notify다. 파일명 복사 직후 본문이 호출한다.</summary>
    public Action<uint>? Notify { get; set; }

    /// <summary>004e65ed의 NUL 종료 대소문자 무시 비교다.</summary>
    public FortSamePath? SamePath { get; set; }

    /// <summary>0041aef0의 이전 경로 삭제 요청이다.</summary>
    public FortDeleteRequest? DeleteRequest { get; set; }

    /// <summary>00460730의 실제 현재 제목 갱신이다.</summary>
    public Action? RefreshTitle { get; set; }

    /// <summary>004bd550의 고정 this 현재 State다. 본문은 반환 뒤 +0x11 바이트만 읽는다.</summary>
    public RawReadOnlyBytesLookup? State { get; set; }

    /// <summary>004414d0의 lastMultiplayerFort/lastEditFort와 그 시점 현재 이름이다.</summary>
    public FortConfigWrite? Config { get; set; }
}

/// <summary>
/// 원본 00460980의 현재 Fort 이름 부모와 004bf630의 같은 manager 파일명 setter를 분리 복원한다.
/// 현재 주소 재조회, 고정 this 주소, 원문 바이트 복사와 진단 뒤 계속을 보존한다. cpppj <c>o/RawFortCurrentName</c>을 옮겼다.
/// </summary>
/// <remarks>2026-11-01 0.128에서 추가했다. 파일 확장자 추가, 실제 삭제, Editor 전체 설치는 범위 밖이다.</remarks>
public static class RawFortCurrentName
{
    /// <summary>manager 파일명 배열의 실제 폭이다(oldPath scratch와 같다).</summary>
    public const int PathBytes = 256;

    /// <summary>실제 도달한 경계만 요구하고 미연결을 임의 성공으로 대체하지 않는다.</summary>
    /// <typeparam name="T">장치 대리자 형태.</typeparam>
    /// <param name="device">요구할 장치.</param>
    private static T Need<T>(T? device) where T : class
    {
        return device ?? throw new InvalidOperationException("Fort현재이름의실제경계미연결");
    }

    /// <summary>같은 실제 헤더의 little endian DWORD만 읽는다. 비정렬 주소에서도 원문 네 바이트만 조립한다.</summary>
    /// <param name="bytes">헤더 바이트.</param>
    /// <param name="offset">DWORD 위치.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset)
    {
        if (offset > bytes.Length || bytes.Length - offset < 4)
        {
            throw new ArgumentOutOfRangeException(null, "Fort현재이름헤더조회범위");
        }
        return RawStorageBytes.Read(bytes, offset);
    }

    /// <summary>원본 헤더 +8 같은 선택한 동일 주소에 DWORD를 쓴다. 다른 필드와 주변 바이트는 유지한다.</summary>
    /// <param name="bytes">헤더 바이트.</param>
    /// <param name="offset">DWORD 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        if (offset > bytes.Length || bytes.Length - offset < 4)
        {
            throw new ArgumentOutOfRangeException(null, "Fort현재이름헤더쓰기범위");
        }
        RawStorageBytes.Put(bytes, offset, value);
    }

    /// <summary>현재 NUL 버퍼의 첫 바이트만 읽는다. 없는 물리 주소를 빈 문자열로 가장하지 않는다.</summary>
    /// <param name="bytes">현재 이름 버퍼.</param>
    private static bool Empty(ReadOnlySpan<byte> bytes)
    {
        if (bytes.IsEmpty)
        {
            throw new ArgumentOutOfRangeException(null, "Fort현재이름첫BYTE범위");
        }
        return bytes[0] == 0;
    }

    /// <summary>원문 strchr처럼 NUL 또는 첫 점을 만날 때까지 읽는다. 검사 뒤에도 원본 source 주소를 유지하며 미종료 물리 입력은 명시 거부한다.</summary>
    /// <param name="bytes">새 이름 바이트.</param>
    private static bool HasDot(ReadOnlySpan<byte> bytes)
    {
        // NUL 뒤 점은 검사하지 않는다
        foreach (byte value in bytes)
        {
            if (value == (byte)'.')
            {
                return true;
            }
            if (value == 0)
            {
                return false;
            }
        }
        throw new ArgumentOutOfRangeException(null, "Fort새이름NUL범위");
    }

    /// <summary>전방 한 바이트 읽기/쓰기 뒤 NUL을 판정하여 자기참조/겹침과 이후 실패 전 부분 효과를 보존한다.</summary>
    /// <param name="target">쓸 대상.</param>
    /// <param name="source">읽을 원본.</param>
    private static void Copy(Span<byte> target, ReadOnlySpan<byte> source)
    {
        // 무제한 C 주소 접근은 실제 공급된 물리 범위에서만 명시적으로 중단한다
        for (int index = 0; ; index++)
        {
            if (index >= source.Length || index >= target.Length)
            {
                throw new ArgumentOutOfRangeException(null, "Fort이름NUL복사물리범위");
            }
            byte value = source[index];
            target[index] = value;
            if (value == 0)
            {
                return;
            }
        }
    }

    /// <summary>실제 현재 owner를 그 시점에 한 번 읽고 고정 Template 주소를 선택한다.</summary>
    /// <param name="globals">현재 owner 조회.</param>
    /// <param name="hooks">헤더 조회.</param>
    private static Span<byte> CurrentHeader(FortCurrentNameGlobals globals, FortCurrentNameHooks hooks)
    {
        return Need(hooks.Header)(Need(globals.Owner)());
    }

    /// <summary>원본은 PathSpec 호출마다 전역 포인터를 선택하되 FileSpec 내용 자체는 변경하지 않는다.</summary>
    /// <param name="name">경로로 바꿀 이름.</param>
    /// <param name="hooks">FileSpec 선택·경로 장치.</param>
    private static ReadOnlySpan<byte> Path(ReadOnlySpan<byte> name, FortCurrentNameHooks hooks)
    {
        Need(hooks.SelectFileSpec)();
        return Need(hooks.PathSpec)(name);
    }

    /// <summary>
    /// 004bf630: 같은 고정 Template 헤더의 manager 진단(429)→중복 진단(430)→현재 manager +c에 NUL 복사→자체 Notify 순서다.
    /// 진단이 현재 owner/manager를 바꾸더라도 this 헤더는 고정하고 manager 값만 다시 읽는다.
    /// </summary>
    /// <param name="slot">고정 Template 슬롯.</param>
    /// <param name="path">복사할 NUL 종료 경로.</param>
    /// <param name="hooks">헤더·파일명·진단·Notify 장치.</param>
    public static void SetFortManagerFileName(uint slot, ReadOnlySpan<byte> path, FortCurrentNameHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        Span<byte> header = Need(hooks.Header)(slot);
        if (Read(header, 0) == 0)
        {
            Need(hooks.Assertion)(429);
        }
        if (Read(header, 4) != 0)
        {
            Need(hooks.Assertion)(430);
        }
        Copy(Need(hooks.ManagerName)(Read(header, 0)), path);
        Need(hooks.Notify)(slot);
    }

    /// <summary>
    /// 00460980: 현재 Fort 이름을 바꾼다. callback 뒤 현재 owner/name을 재조회하고 원본 명령이 캡처한 readonly만 유지한다.
    /// 새 이름은 NUL을 포함한 실제 원본 버퍼다. 원문 미초기화 oldpath 256바이트는 명시 scratch로 공급하며 초기화하거나 복제하지 않는다.
    /// 물리 범위를 넘으면 앞서 쓴 바이트/필드 효과를 보존한 채 예외로 중단한다.
    /// </summary>
    /// <param name="name">새 이름(NUL 포함).</param>
    /// <param name="oldPathScratch">이전 경로를 담는 256바이트 scratch.</param>
    /// <param name="globals">현재 Client 전역.</param>
    /// <param name="hooks">파일·장치·Snapshot·Notify 경계.</param>
    public static void SetCurrentName(ReadOnlySpan<byte> name, Span<byte> oldPathScratch, FortCurrentNameGlobals globals, FortCurrentNameHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (oldPathScratch.Length != PathBytes)
        {
            throw new ArgumentException("Fort 이전 경로 scratch는 256BYTE여야 한다");
        }
        if (Read(CurrentHeader(globals, hooks), 0) != 0)
        {
            if (Empty(Need(hooks.CurrentName)()))
            {
                Need(hooks.Assertion)(39);
            }
            Copy(oldPathScratch, Path(Need(hooks.CurrentName)(), hooks));
        }
        if (HasDot(name))
        {
            Need(hooks.Assertion)(88);
        }
        Span<byte> beforeCopy = CurrentHeader(globals, hooks);
        if (Read(beforeCopy, 0) != 0 && Read(beforeCopy, 8) == 0)
        {
            Need(hooks.Snapshot)();
        }
        Copy(Need(hooks.CurrentName)(), name);
        Span<byte> afterCopy = CurrentHeader(globals, hooks);
        if (Read(afterCopy, 0) != 0)
        {
            // 이후 callback이 바꿔도 원본 ESI에 캡처한 값만 마지막 관문에 쓴다
            uint savedReadOnly = Read(afterCopy, 8);
            Put(afterCopy, 8, 0);
            if (Empty(Need(hooks.CurrentName)()))
            {
                Need(hooks.Assertion)(39);
            }
            // PathSpec callback이 현재 owner를 바꿀 수 있으므로 그 뒤 다시 읽는다
            ReadOnlySpan<byte> path = Path(Need(hooks.CurrentName)(), hooks);
            SetFortManagerFileName(Need(globals.Owner)(), path, hooks);
            Need(hooks.Snapshot)();
            if (savedReadOnly == 0)
            {
                ReadOnlySpan<byte> newPath = Path(name, hooks);
                if (!Need(hooks.SamePath)(oldPathScratch, newPath))
                {
                    Need(hooks.DeleteRequest)(oldPathScratch);
                }
            }
        }
        Need(hooks.RefreshTitle)();
        uint dirty = Need(globals.Dirty)();
        uint fort = Need(globals.Fort)();
        Need(globals.SetDirty)(dirty + 1u);
        if (fort != 0)
        {
            if (Need(globals.Editor)() != 0)
            {
                Need(hooks.Config)("lastEditFort", Need(hooks.CurrentName)());
                return;
            }
            if (Need(globals.Mission)() != 0)
            {
                return;
            }
            uint slot = Need(globals.Owner)();
            if (Read(Need(hooks.Header)(slot), 0) == 0)
            {
                return;
            }
            ReadOnlySpan<byte> state = Need(hooks.State)(slot);
            if (state.Length <= 0x11)
            {
                throw new ArgumentOutOfRangeException(null, "Fort현재State관문BYTE범위");
            }
            if (state[0x11] == 0)
            {
                Need(hooks.Config)("lastMultiplayerFort", Need(hooks.CurrentName)());
            }
        }
        if (Need(globals.Editor)() != 0)
        {
            Need(hooks.Config)("lastEditFort", Need(hooks.CurrentName)());
        }
    }
}

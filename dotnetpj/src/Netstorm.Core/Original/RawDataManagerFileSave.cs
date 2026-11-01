using static Netstorm.Core.Original.RawStorageBytes;

namespace Netstorm.Core.Original;

/// <summary>
/// 저장 억제·로컬 소유자·숨김 표를 이미지 생성 효과 뒤에 다시 읽는 원본 전역의 현재 값이다(원본 <c>DataManagerFileSaveState</c>).
/// 값을 미리 복사하지 않고 읽는 시점마다 조회한다.
/// </summary>
/// <remarks>2026-11-01 0.128에서 추가했다. cpppj는 전역 참조를 들고 있고 C#은 같은 읽기 시점을 위해 조회 대리자를 둔다.</remarks>
public sealed class DataManagerFileSaveState
{
    /// <summary>0054db30의 현재 비영 저장 억제 값이다.</summary>
    public Func<uint>? ReadOnlyForts { get; set; }

    /// <summary>00540c70의 현재 숨김 표 인덱스다.</summary>
    public Func<uint>? LocalOwner { get; set; }

    /// <summary>0052f81e부터의 실제 현재 숨김 바이트 표다.</summary>
    public Func<ReadOnlyMemory<byte>>? HiddenPlayers { get; set; }
}

/// <summary>이미지를 manager와 초기 MarkStream20에 쓰는 0044d640이다.</summary>
/// <param name="manager">저장할 DataManager.</param>
/// <param name="image">초기 MarkStream 20바이트(첫 네 DWORD 0, mark -1). 이미지 생성 뒤 가상 이름/권한 호출에서 같은 20바이트를 다시 읽고 바꿀 수 있도록 Memory로 빌려 준다.</param>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public delegate void DataManagerImageWriter(Span<byte> manager, Memory<byte> image);

/// <summary>현재 manager 가상 +0의 파일 이름을 읽는다.</summary>
/// <param name="manager">대상 DataManager.</param>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public delegate string DataManagerFileNameReader(Span<byte> manager);

/// <summary>저장 규칙을 대신 계산하지 않는 이미지·가상 이름·CRT·파일 쓰기 경계다(원본 <c>DataManagerFileSaveHooks</c>).</summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public sealed class DataManagerFileSaveHooks
{
    /// <summary>0044d640이 같은 manager와 초기 MarkStream20에 쓴다.</summary>
    public DataManagerImageWriter? Image { get; set; }

    /// <summary>현재 manager 가상 +0이며 권한과 쓰기에서 각각 조회한다.</summary>
    public DataManagerFileNameReader? FileName { get; set; }

    /// <summary>CRT chmod의 현재 이름과 0x180 요청이다. 반환값은 원본처럼 쓰지 않는다.</summary>
    public Action<string, uint>? Permissions { get; set; }

    /// <summary>현재 이름·앞서 보관한 이미지 포인터·쓰기 인자 길이의 0041bb90이다.</summary>
    public Action<string, uint, uint>? Write { get; set; }

    /// <summary>정상 반환에서 보관한 이미지 포인터를 한 번만 반납한다.</summary>
    public Action<uint>? Release { get; set; }
}

/// <summary>
/// 10.78 DataManager 저장 부모(0044d8d0)를 실제 이미지·현재 전역·파일 장치와 연결한다.
/// 현재 파일 이름과 이미지 포인터·길이의 서로 다른 읽기 시점을 원본 저장 부모와 맞춘다. cpppj <c>o/RawDataManagerFileSave</c>를 옮겼다.
/// </summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public static class RawDataManagerFileSave
{
    /// <summary>DataManager 한 개의 실제 최소 물리 폭이다.</summary>
    private const int ManagerBytes = 12;

    /// <summary>초기 MarkStream 한 개의 실제 폭(MemStream 16 + mark 4)이다.</summary>
    private const int ImageBytes = 20;

    /// <summary>CRT chmod에 요청하는 권한(읽기·쓰기)이다.</summary>
    private const uint WritablePermissions = 0x180;

    /// <summary>
    /// 0044d8d0: 이미지 생성→현재 저장/숨김 관문→두 이름 조회/권한/쓰기→이미지 반납이다.
    /// 저장 억제 여부와 무관하게 이미지를 먼저 생성하며 현재 포인터의 물리 계약만 호스트에서 검사한다.
    /// 호스트 예외에서도 이미지 소유 할당을 반납하며 원본 SEH의 모든 변이를 복원했다는 뜻은 아니다.
    /// </summary>
    /// <param name="manager">저장할 DataManager.</param>
    /// <param name="state">저장 억제·숨김 전역의 현재 값.</param>
    /// <param name="hooks">이미지·이름·권한·쓰기·반납 장치.</param>
    public static void Save(Span<byte> manager, DataManagerFileSaveState state, DataManagerFileSaveHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (manager.Length < ManagerBytes)
        {
            throw new ArgumentException("DataManager 실제 파일 저장의 manager 누락");
        }
        // 원본 첫 네 DWORD 0/mark -1이다
        byte[] image = new byte[ImageBytes];
        image.AsSpan(16, 4).Fill(0xff);
        uint pointer = 0;
        bool captured = false;
        bool released = false;

        // 직렬화 완료 뒤에는 보관한 포인터를, 생성 중 호스트 예외에서는 현재 포인터를 한 번만 반납한다
        void Release()
        {
            if (released)
            {
                return;
            }
            released = true;
            uint current = captured ? pointer : Read(image, 0);
            if (current != 0)
            {
                Need(hooks.Release, "DataManager 실제 파일 저장의 하위 장치 미연결")(current);
            }
        }

        try
        {
            Need(hooks.Image, "DataManager 실제 파일 저장의 하위 장치 미연결")(manager, image);
            pointer = Read(image, 0);
            captured = true;
            if (Need(state.ReadOnlyForts, "DataManager 실제 파일 저장의 하위 장치 미연결")() == 0)
            {
                uint owner = Need(state.LocalOwner, "DataManager 실제 파일 저장의 하위 장치 미연결")();
                ReadOnlyMemory<byte> hidden = Need(state.HiddenPlayers, "DataManager 실제 파일 저장의 하위 장치 미연결")();
                if (owner >= (uint)hidden.Length)
                {
                    throw OutOfRange("DataManager 저장의 현재 숨김 표 물리 인덱스 범위");
                }
                if (hidden.Span[(int)owner] == 0)
                {
                    DataManagerFileNameReader fileName = Need(hooks.FileName, "DataManager 실제 파일 저장의 하위 장치 미연결");
                    Need(hooks.Permissions, "DataManager 실제 파일 저장의 하위 장치 미연결")(fileName(manager), WritablePermissions);
                    // 원본은 둘째 가상 이름 조회 전에 길이 인자를 보관한다
                    uint length = Read(image, 8);
                    string name = fileName(manager);
                    Need(hooks.Write, "DataManager 실제 파일 저장의 하위 장치 미연결")(name, pointer, length);
                }
            }
            Release();
        }
        catch
        {
            Release();
            throw;
        }
    }
}

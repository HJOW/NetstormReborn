using static Netstorm.Core.Original.RawStorageBytes;

namespace Netstorm.Core.Original;

/// <summary>원본 하위 호출의 효과를 같은 헤더/manager 소유자에 연결한다(원본 <c>TemplateLoadHooks</c>). 선택하지 않은 경계는 요구하지 않는다.</summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public sealed class TemplateLoadHooks
{
    /// <summary>현재 manager 가상 +0의 같은 파일명이다.</summary>
    public Func<uint, string>? FileName { get; set; }

    /// <summary>CRT stricmp==0 비교다. 파일명 문자열의 별도 사본을 만들지 않는다.</summary>
    public Func<string, string, bool>? SameName { get; set; }

    /// <summary>isCalled(_fileName) 진단이다. 뒤에 원본은 자연 반환한다.</summary>
    public Action? WrongName { get; set; }

    /// <summary>실제 operator_new(276)의 DWORD 포인터/NULL이다.</summary>
    public Func<uint, uint>? Allocate { get; set; }

    /// <summary>생성자 뒤 EAX로 돌려준 같은 manager 포인터다.</summary>
    public Func<uint, string, uint>? Construct { get; set; }

    /// <summary>0041af80의 읽기 전용 판정이다. 파일 존재 판정과 다르다.</summary>
    public Func<string, bool>? ReadOnly { get; set; }

    /// <summary>현재 header.ptr에 대한 0044d980이다.</summary>
    public Func<uint, bool>? ReadFile { get; set; }

    /// <summary>this==Template9[currentIndex]의 부작용 없는 현재 주소 비교다.</summary>
    public Func<bool>? IsCurrent { get; set; }

    /// <summary>선택된 현재 Template의 00460730 창 제목 갱신이다.</summary>
    public Action? RefreshTitle { get; set; }

    /// <summary>읽기 실패 시 base DataManager 소멸과 저장 포인터 free를 순서대로 호출한다.</summary>
    public TemplateLifetimeHooks Lifetime { get; set; } = new();
}

/// <summary>
/// Template manager 생성자(004bcc60)와 가상 +0 파일명(004bcca0), 기존 파일을 유지하는 바깥 Load(004bf550)의 원본 제어 순서다.
/// 원본이 보존하는 파일명 뒤 바이트와 헤더 부가 필드를 임의로 초기화하지 않는다. cpppj <c>o/RawTemplateLoad</c>를 옮겼다.
/// </summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public static class RawTemplateLoad
{
    /// <summary>10.78 Template manager의 실제 폭(276)이다.</summary>
    private const int ManagerBytes = 276;

    /// <summary>Template 기록 한 개의 실제 폭이다.</summary>
    private const int HeaderBytes = 20;

    /// <summary>manager 안 파일명 배열의 시작 위치다.</summary>
    private const int NameOffset = 12;

    /// <summary>manager 안 파일명 배열의 실제 폭이다(NUL 포함).</summary>
    private const int NameBytes = 256;

    /// <summary>생성자가 쓰는 10.78 DataManager 파생 가상 표 식별자다.</summary>
    private const uint TemplateVtable = 0x514690;

    /// <summary>
    /// 004bcc60: manager 276바이트의 지정 DWORD와 NUL까지의 이름만 쓴다. global은 005c89c0의 같은 4바이트(직렬화 버전)다.
    /// 원본이 쓰지 않는 파일명 뒤 공간과 manager 밖 바이트는 보존한다. 파일명 입력은 출력 manager/global과 겹치지 않아야 하며 256바이트 밖 원본 쓰기는 예외로 거부한다.
    /// </summary>
    /// <param name="manager">Template manager 276바이트.</param>
    /// <param name="fileName">파일 이름. 첫 NUL까지만 쓴다.</param>
    /// <param name="global">직렬화 버전 전역 4바이트. 0으로 쓴다.</param>
    public static void ConstructManager(Span<byte> manager, string fileName, Span<byte> global)
    {
        int nul = fileName.IndexOf('\0');
        int length = nul < 0 ? fileName.Length : nul;
        if (manager.Length < ManagerBytes || length >= NameBytes || global.Length != 4)
        {
            throw new ArgumentException("Template 생성자의 실제 manager/파일명 범위");
        }
        Put(manager, 4, 0);
        Put(manager, 8, 0);
        Put(manager, 0, TemplateVtable);
        Put(manager, 0x10c, 0);
        Put(manager, 0x110, 0xffffffffu);
        Put(global, 0, 0);
        // 입력 C 문자열의 첫 NUL까지 한 바이트씩 순서대로 복사한다
        for (int index = 0; index <= length; index++)
        {
            manager[NameOffset + index] = index == length ? (byte)0 : (byte)fileName[index];
        }
    }

    /// <summary>가상 +0의 004bcca0: 같은 manager +12의 NUL 종료 파일명을 빌린다. 별도 metadata 문자열 대신 실제 manager 배열에서 읽으므로 이름이 바뀌면 같은 바이트를 본다.</summary>
    /// <param name="manager">Template manager 276바이트.</param>
    public static string FileName(ReadOnlySpan<byte> manager)
    {
        if (manager.Length < ManagerBytes)
        {
            throw new ArgumentException("Template 파일명 getter의 manager276 누락");
        }
        return TerminatedText(manager.Slice(NameOffset, NameBytes), "Template 파일명의 실제 NUL 누락");
    }

    /// <summary>
    /// 004bf550: 기존 같은 파일은 그대로 두고 빈 헤더에만 할당/읽기를 수행한다. 실패는 ptr만 0으로 쓴다.
    /// 하위 효과 뒤 원본이 다시 읽는 ptr/readonly/current를 같은 헤더에서 재조회한다. C++ 예외 정리 정책은 실제 소유 어댑터가 담당하며 원본 정상 반환과 구분한다.
    /// </summary>
    /// <param name="header">Template 기록 20바이트.</param>
    /// <param name="fileName">요청 파일 이름.</param>
    /// <param name="hooks">할당·생성·읽기·제목 장치.</param>
    public static void Load(Span<byte> header, string fileName, TemplateLoadHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        if (header.Length != HeaderBytes)
        {
            throw new ArgumentException("Template Load의 실제20BYTE 헤더 누락");
        }
        uint old = Read(header, 0);
        if (old != 0)
        {
            string name = Need(hooks.FileName, "Template Load의 실제 하위 효과 미연결")(old);
            if (!Need(hooks.SameName, "Template Load의 실제 하위 효과 미연결")(name, fileName))
            {
                Need(hooks.WrongName, "Template Load의 실제 하위 효과 미연결")();
            }
            return;
        }
        Put(header, 4, 0);
        uint manager = Need(hooks.Allocate, "Template Load의 실제 하위 효과 미연결")(ManagerBytes);
        if (manager != 0)
        {
            manager = Need(hooks.Construct, "Template Load의 실제 하위 효과 미연결")(manager, fileName);
        }
        Put(header, 0, manager);
        bool readOnly = Read(header, 8) != 0
            || Need(hooks.ReadOnly, "Template Load의 실제 하위 효과 미연결")(Need(hooks.FileName, "Template Load의 실제 하위 효과 미연결")(manager));
        Put(header, 8, readOnly ? 1u : 0u);
        Put(header, 16, 0);
        if (Need(hooks.ReadFile, "Template Load의 실제 하위 효과 미연결")(Read(header, 0)))
        {
            if (Read(header, 0) != 0 && Need(hooks.IsCurrent, "Template Load의 실제 하위 효과 미연결")())
            {
                Need(hooks.RefreshTitle, "Template Load의 실제 하위 효과 미연결")();
            }
            return;
        }
        uint failed = Read(header, 0);
        if (failed != 0)
        {
            Need(hooks.Lifetime.Destroy, "Template Load의 실제 하위 효과 미연결")(failed);
            Need(hooks.Lifetime.Release, "Template Load의 실제 하위 효과 미연결")(failed);
        }
        Put(header, 0, 0);
    }
}

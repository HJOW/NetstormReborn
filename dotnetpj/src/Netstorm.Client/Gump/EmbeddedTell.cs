using System.Text;
using Netstorm.Assets;

namespace Netstorm.Client.Gump; // 일반 팝업 이력/param과 별개인 원본 내장 설명 경로다.

/// <summary>현재 Mission→매번 새 Tell→실제 HTML 부모/현재 serial의 선택 장치.</summary>
public sealed class EmbeddedTellHooks
{
    /// <summary>현재 미션 섹션.없는 섹션null과 존재하는 빈 본문을 구별한다.</summary>
    public Func<string, string?>? Mission { get; set; }
    /// <summary>매번 새로 읽는 실제 tell 설정 섹션.</summary>
    public Func<string, string?>? Fallback { get; set; }
    /// <summary>두 조회가 실패했을 때만 원문 경고를 보낸다.</summary>
    public Action<string>? Missing { get; set; }
    /// <summary>실제 new0x40a4/HTML 부모 생성.</summary>
    public Func<uint>? Construct { get; set; }
    /// <summary>같은 HTML 부모의 실제 파싱/표시 설치.</summary>
    public Action<uint, string>? SetHtml { get; set; }
    /// <summary>본문 소유권 반납 뒤 현재 부모+2c를 읽는다.</summary>
    public Func<uint, uint>? Serial { get; set; }
}
/// <summary>cpppj/src/client/EmbeddedTell.h/.cpp의004c8f60 선택/생성/현재 반환 순서.</summary>
public static class EmbeddedTell
{
    /// <summary>실제 선택된 장치만 요구한다.</summary>
    private static T Require<T>(T? hook) where T : Delegate => hook ?? throw new InvalidOperationException("내장 TELL 실제 선택 장치 미연결");
    /// <summary>현재 원문 NUL 문자열만 빌린다.</summary>
    private static string CText(string text) { int end = text.IndexOf('\0'); return end < 0 ? text : text[..end]; }
    /// <summary>이름의 등록 객체/param/BLANK/팝업 생략을 추가하지 않고 현재 serial 또는 조회 실패0을 반환한다.</summary>
    public static uint ShowEmbeddedTell(string name, EmbeddedTellHooks hooks) { name = CText(name); string? body = hooks.Mission?.Invoke(name); if (body is null) body = Require(hooks.Fallback)(name); if (body is null) { Require(hooks.Missing)(name); return 0; } uint token = Require(hooks.Construct)(); if (token == 0) throw new OutOfMemoryException("내장 TELL 부모 할당 실패"); Require(hooks.SetHtml)(token, body); body = null; return Require(hooks.Serial)(token); }
    /// <summary>원문 좁은 키를 실제 Config의 Unicode 경계에서만 해독한다.</summary>
    private static string ConfigName(string raw) { byte[] bytes = Encoding.Latin1.GetBytes(raw); return bytes.AsSpan().StartsWith(new byte[] { 0xef, 0xbb, 0xbf }) ? Encoding.UTF8.GetString(bytes.AsSpan(3)) : OriginalText.DecodeWindows1252(bytes); }
    /// <summary>Config 내부 UTF-8 std::string의 원문 BYTE를 HTML의 Latin1 표현으로 옮긴다.</summary>
    private static string HtmlBytes(string text) => Encoding.Latin1.GetString(Encoding.UTF8.GetBytes(text));
    /// <summary>같은 현재 설정/파일을 매번 읽고 임시 ConfigText를 UI 생성 전에 끝낸다.전체 ConfigInterface 등록부는 후속이다.</summary>
    public static uint ShowEmbeddedTellInConfig(string name, ConfigStore config, Func<string, byte[]?> read, Func<string, string?>? mission, EmbeddedTellHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(config); ArgumentNullException.ThrowIfNull(read); var selected = new EmbeddedTellHooks { Mission = mission is null ? null : key => mission(ConfigName(key)) is { } text ? HtmlBytes(text) : null, Fallback = key => { byte[]? bytes = read(config.ExpandSpec("missionSpec", "tell")); var temporary = bytes is null ? new ConfigText("") : ConfigText.FromFileBytes(bytes); return temporary.Section(ConfigName(key)) is { } section ? HtmlBytes(section) : null; }, Missing = hooks.Missing, Construct = hooks.Construct, SetHtml = hooks.SetHtml, Serial = hooks.Serial }; return ShowEmbeddedTell(name, selected);
    }
}

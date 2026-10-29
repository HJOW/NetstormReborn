using System.Text;
using Netstorm.Assets;
using Netstorm.Client.Gump;
using Netstorm.Client.Scripting;

namespace Netstorm.Tests; // 실제 HTML 생성기/메타/Config/링크/메뉴/모달을 같은 공통 UI에 연결한다.

/// <summary>전체 메뉴 호스트와 구별한 헤드리스 조립.배경/뷰는 명시 입력이고 글꼴은 보호 캐시다.</summary>
internal sealed class HtmlWindowFactoryScene
{
    /// <summary>같은 실제 장식/돌버튼/동적본문/메뉴·공통 입력/수명.</summary>
    internal HtmlWindowWidgetScene Ui { get; } = new();
    /// <summary>현재 실제 설정 층/치환.</summary>
    internal ConfigStore Config { get; } = new();
    /// <summary>같은80칸 문자열 수명.</summary>
    internal ScriptStringRing Strings { get; } = new();
    /// <summary>같은 현재 파서/배경 전역.</summary>
    internal HtmlTextParserGlobals Globals { get; } = new();
    /// <summary>현재 파서의 실제 명령/조건/그림/배경 효과.</summary>
    internal HtmlTextParserHooks Effects { get; } = new();
    /// <summary>같은 실제 부모 링크 소유자.</summary>
    internal HtmlWindowTextParser Parser { get; }
    /// <summary>현재 실제 메타 장치.</summary>
    internal HtmlWindowMetadataHooks MetaHooks { get; }
    /// <summary>같은 실제 메타 처리기.</summary>
    internal HtmlWindowMetadata Metadata { get; }
    /// <summary>현재 실제 전체 생성 장치.</summary>
    internal HtmlWindowFactoryHooks Hooks { get; }
    /// <summary>같은 실제 전체 생성기.</summary>
    internal HtmlWindowFactory Factory { get; }
    /// <summary>같은 실제 모달 표/최소 깊이.</summary>
    internal GumpModal Modal { get; }
    /// <summary>실제 명령 조회 순서/원문.</summary>
    internal List<string> Lookups { get; } = [];
    /// <summary>실제 코드0 진단 관측.</summary>
    internal List<string> Diagnostics { get; } = [];
    /// <summary>실제 OkButton 조회 횟수.</summary>
    internal int FallbackCalls { get; private set; }
    /// <summary>현재 같은 기본/파생 조회.</summary>
    private GumpCreateNode? Find(uint token) => Ui.Common.Nodes.GetValueOrDefault(token);
    /// <summary>원문 바이트/현재 설정·같은 실제 링크/메뉴/모달을 연결한다.</summary>
    internal HtmlWindowFactoryScene()
    {
        Ui.Now = 100.5; Ui.Globals[0x563e38] = 1; Ui.Globals[0x5cab30] = 0; Ui.Globals[0x55a480] = 0; Ui.Globals[0x5429ec] = 0x020000be; Config.Push(new ConfigText("caption=Storm Islands\nOkButton=$Button=Okay,Config,0 \n")); HtmlConditionEffect.BindHtmlConditionConfig(Effects, Config); HtmlTextParser.BindHtmlPictureText(Effects); HtmlTextParser.BindHtmlBackground(Effects, Globals); Parser = new(t => Find(t) as HtmlWindowGumpNode, Globals, Effects);
        MetaHooks = new() { Node = t => Find(t) as HtmlWindowGumpNode, Commands = new() { Lookup = text => { Lookups.Add(text); return text switch { "Config" => 0x0200006dU, "Warning" => 0x02000102U, "Tell" => 0x02000095U, _ => uint.MaxValue }; }, Unknown = text => Diagnostics.Add(text) }, WallSeconds = () => Ui.Now, Read = address => Ui.Globals[address], Queue = (c, a, b) => Ui.Queue.Push(c, unchecked((int)a), unchecked((int)b)) }; HtmlWindowMetadata.BindHtmlWindowMetadataDevices(MetaHooks, Ui.Window, Strings, Ui.Common.Lifetime); HtmlWindowMetadata.BindHtmlWindowMetadataMenus(MetaHooks, Ui.MenuWidgets, Ui.MenuControls); Metadata = new(MetaHooks); HtmlWindowFit.BindHtmlWindowFitMenus(Ui.FitHooks, Ui.Menu); Modal = new(Find, Ui.Common.LifeState, Ui.Common.Input, Ui.Common.Lifetime, Ui.Common.Tree, () => { });
        Hooks = new() { Create = Ui.Create, Node = MetaHooks.Node, Substitute = (text, _) => Config.Expand(text), Fallback = () => { FallbackCalls++; return Config.Get("OkButton") ?? throw new InvalidOperationException("실제 OkButton 설정 누락"); }, WallSeconds = MetaHooks.WallSeconds, Attach = (t, x, y, p, m) => Ui.Common.TreeHooks.Attach!(t, x, y, p, m), Modal = Modal.SetModal, Read = MetaHooks.Read }; HtmlWindowFactory.BindHtmlWindowFactoryDevices(Hooks, Ui.Window, Metadata, Parser, Ui.Common.Lifetime); Factory = new(Ui.State, Hooks); var release = Ui.Common.LifetimeHooks.Release!; Ui.Common.LifetimeHooks.Release = t => { release(t); Parser.Forget(t); };
    }
    /// <summary>현재 원문 BYTE를 실제8192BYTE 메타 입력으로 만든다.</summary>
    internal static byte[] Buffer(string text, byte fill = 0) { byte[] bytes = Enumerable.Repeat(fill, 8192).ToArray(); Encoding.Latin1.GetBytes(text, bytes); bytes[text.Length] = 0; return bytes; }
    /// <summary>같은 전체 공장이 반환한 serial을 현재 실제 부모로 찾는다.</summary>
    internal uint Show(string source, uint suppress = 0, uint width = 400, uint height = 0) => Ui.Reference(Factory.Show(source, suppress, width, height));
    /// <summary>현재 같은 실제 본문의 완료 입력 줄을 조각으로 관측한다.</summary>
    internal string BodyText(uint root) { var state = Ui.Buffers.Binding(Ui.Body(root).Buffer).State; return string.Concat(state.HardLines.Take(state.LineCount).SelectMany(head => StyleTextTestSupport.Chain(Ui.Fragments, head)).Select(t => StyleTextTestSupport.Text(Ui.Fragments.Fragments[t]))); }
    /// <summary>같은 부모/미부착 메뉴도 실제 공통 수명으로 정리한다.</summary>
    internal void Close() {
        // 메타 해석 도중 실패한 현재 controlled 메뉴 예약 칸도 반납한다.
        while (Ui.Menu.Current() != 0) Ui.Common.Lifetime.Kill(Ui.Menu.TakeCurrent());
        // 현재 등록된 모든 HTML 부모만 요청하고 자식은 실제 공통 수명이 처리한다.
        foreach (uint token in Ui.Common.Create.Entries.ToArray()) if (token != 0 && Find(token) is HtmlWindowGumpNode) Ui.Common.Lifetime.Kill(token); Ui.Common.Lifetime.Cleanup();
    }
}

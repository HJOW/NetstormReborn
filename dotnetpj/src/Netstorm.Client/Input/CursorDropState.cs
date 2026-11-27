// 원문 내려놓기·취득·취소가 공유하는 전체 DWORD와 float 전역이다.
using Netstorm.Core.Original;

namespace Netstorm.Client.Input;

/// <summary>RawCursorDrop.h의 데이터만 소유한다. 실제 Drop 본체/외부 효과와 구분한다.</summary>
public sealed class CursorDropState
{
    /// <summary>5325c8의 중첩 잠금 깊이다. 단순 raw 입력은0이며 실제 Gameplay 최초값은1이다.</summary>
    public uint LockDepth { get; set; }
    /// <summary>현재 배치 허용 DWORD다.</summary>
    public uint Legal { get; set; }
    /// <summary>현재 선택 TYPE 번호이며 별도 abstractType과 구분한다.</summary>
    public uint Type { get; set; }
    /// <summary>타입/패턴/명령에 전달하는 첫 인자다.</summary>
    public uint Argument { get; set; }
    /// <summary>현재 방향/회전 인자다.</summary>
    public uint Direction { get; set; }
    /// <summary>Form으로 돌려쓸 원래 입력 WORD를 보관하는 DWORD다.</summary>
    public uint InputFlags { get; set; }
    /// <summary>취득 호출의 별도 전체 비용/역할 플래그다.</summary>
    public uint CostFlags { get; set; }
    /// <summary>55a2a8의 전체 미리보기 DWORD를 같은 현재 주소에서 빌린다.</summary>
    public UIntBox PreviewBox { get; } = new();
    /// <summary>실제 Pool 소비 전까지 자르지 않는 현재 미리보기 값이다.</summary>
    public uint Preview { get => PreviewBox.Value; set => PreviewBox.Value = value; }
    /// <summary>현재 단정도 월드 X다.</summary>
    public float X { get; set; }
    /// <summary>현재 단정도 월드 Y다.</summary>
    public float Y { get; set; }
    /// <summary>55a27c의 현재 허용 Gump 토큰이다.</summary>
    public uint LegalGump { get; set; }
    /// <summary>55a274의 현재 마나/사정거리 프로세스이며 currentGump와 다르다.</summary>
    public uint CursorGump { get; set; }
    /// <summary>일반 World 초기화에서도 보존하는 별도 기록 관문이다.</summary>
    public uint Recording { get; set; }
    /// <summary>일반 World 초기화에서도 보존하는 별도 기록기 식별자다.</summary>
    public uint Recorder { get; set; }
    /// <summary>내려놓기 소비자가 읽는 현재 전투 입력이다.</summary>
    public uint Battle { get; set; }
    /// <summary>공간/소유권의 현재 차단 입력이다.</summary>
    public uint Blocked { get; set; }
    /// <summary>현재 로컬 플레이어 번호다.</summary>
    public uint LocalPlayer { get; set; }
    /// <summary>55a2f8의 hotspot 보정 화면 X이며 원시 mouse와 구분한다.</summary>
    public int ScreenX { get; set; }
    /// <summary>55a2fc의 hotspot 보정 화면 Y다.</summary>
    public int ScreenY { get; set; }
    /// <summary>비용 그림과 글자를 합한 signed 폭이다.</summary>
    public int CursorWidth { get; set; }

    /// <summary>같은 실제 객체의 모든 raw 입력을 struct{}처럼0으로 쓴다.</summary>
    public void Reset()
    {
        LockDepth = Legal = Type = Argument = Direction = InputFlags = CostFlags = Preview = 0;
        X = Y = 0; LegalGump = CursorGump = Recording = Recorder = Battle = Blocked = LocalPlayer = 0;
        ScreenX = ScreenY = CursorWidth = 0;
    }
    /// <summary>기존 일반 World 의미만 적용한다. 두 기록 전역을 보존하고 최초 잠금1로 준비한다.</summary>
    public void ResetForGeneralWorld()
    {
        uint recording = Recording, recorder = Recorder; Reset(); LockDepth = 1; Recording = recording; Recorder = recorder;
    }
}

using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Tests; // 실제 RLE와 같은 노드 저장소로 배경 검사 입력만 만든다.

/// <summary>두 레이어의 독립 SHP·명암표와 원본 기본 생성기를 공유하는 장면이다.
/// 근거: cpppj/tests/GumpBackgroundTestAssets.h·GumpBackgroundDrawTests.cpp.</summary>
internal static class GumpBackgroundTestAssets
{
    /// <summary>독립 입력의 WORD/DWORD를 리틀 엔디언으로 쓴다.</summary>
    private static void Put(List<byte> raw, int offset, uint value, int size = 4)
    {
        // 입력 메타 바이트만 작성하며 기대값은 여기서 만들지 않는다.
        for (int i = 0; i < size; i++) raw[offset + i] = unchecked((byte)(value >> (i * 8)));
    }

    /// <summary>실제 추가 헤더·3×3 RLE와 레이어별 다른 반복 치수/투명 중앙을 가진 SHP다.</summary>
    public static ShapeDatabase Shapes()
    {
        var raw = new List<byte>(new byte[24]); Put(raw, 0, 0x30312e31); Put(raw, 4, 2);
        // 두 물리 프레임의 메타 크기는 실제 출력 그림 크기와 다르다.
        for (int frame = 0; frame < 2; frame++)
        {
            int prefix = raw.Count; raw.AddRange(new byte[60]); Put(raw, 8 + frame * 8, (uint)(prefix + 36));
            Put(raw, prefix + 24, frame != 0 ? 5U : 3U, 2); Put(raw, prefix + 26, frame != 0 ? 4U : 3U, 2);
            Put(raw, prefix + 36, 2, 2); Put(raw, prefix + 38, 2, 2); Put(raw, prefix + 52, 2); Put(raw, prefix + 56, 2);
            byte[] colors = [1, 2, 1, 3, 4, 3, 1, 2, 1];
            // 각 행 끝0을 가진 실제 RLE를 만든다.
            for (int y = 0; y < 3; y++)
            {
                // 두 번째 레이어의 중앙만 skip이고 나머지는 한 픽셀 반복 run이다.
                for (int x = 0; x < 3; x++)
                {
                    if (frame != 0 && x == 1 && y == 1) raw.AddRange([1, 1]); else raw.AddRange([2, (byte)(colors[y * 3 + x] + frame * 4)]);
                }
                raw.Add(0);
            }
        }
        return new(raw.ToArray());
    }

    /// <summary>손계산할 수 있도록 소스 번호에10/20을 더하는 독립 명암표.</summary>
    public static GumpShadeMaps Maps()
    {
        var maps = new GumpShadeMaps();
        // 모든 색의 BYTE 감김을 실제 치환표와 같이 만든다.
        for (int i = 0; i < 256; i++) { maps.Bright[i] = unchecked((byte)(i + 10)); maps.Dark[i] = unchecked((byte)(i + 20)); }
        return maps;
    }

    /// <summary>명시 타입82는 기본 레이어만,83은 추가 레이어를 허용한다.</summary>
    public static GumpBackgroundAssets Assets() => new(Shapes(), [new(82, 0, 1, 0), new(83, 0, 1, 0x440000)]);

    /// <summary>등록/위치/캐시/배경 파생이 같은 안정된 객체를 사용하는 장면.</summary>
    internal sealed class Scene
    {
        /// <summary>현재 등록 표/판본/화면.</summary>
        public GumpCreateState State { get; } = new() { ScreenWidth = 32, ScreenHeight = 24 };
        /// <summary>기본 루트 토큰1.</summary>
        public GumpCreateNode Root { get; } = new();
        /// <summary>배경 파생 토큰2.</summary>
        public GumpBackgroundNode Node { get; } = new();
        /// <summary>실제 기본 생성/등록 처리기.</summary>
        public GumpCreate Factory { get; }
        /// <summary>같은 객체의 실제 위치/경계 계산.</summary>
        public GumpCollection Collection { get; }
        /// <summary>기본/파생 저장소를 같은 객체로 조회한다.</summary>
        private GumpCreateNode? Find(uint token) => token == 1 ? Root : token == 2 ? Node : null;
        /// <summary>원본 기본 루트 초기화를 사용한다.</summary>
        public Scene(OriginalEdition edition = OriginalEdition.Patch1078)
        {
            State.Collection.Edition = edition; Factory = new(State, new() { Node = Find, Allocate = _ => 1 });
            Collection = new(State.Collection, Factory.CollectionHooks()); Factory.InitializeRoot();
        }
    }
}

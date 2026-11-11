// 두 보호 PE의 독립 초기 프레임/stamp 결과를 같은 raw pool/두 난수/공유 Choose와 비교한다.
using System;
using System.Buffers.Binary;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Xunit;

// 원본 지형 도장 x86 관찰 대조 테스트를 담는 네임스페이스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
namespace Netstorm.Core.Tests;

// RawTerrainStamp의 FrameAt과 Stamp를 검증하는 테스트 클래스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
public sealed class TerrainStampTests
{
    // 원본 함수에 넣은 실제 섬 타입 번호 상수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private const uint TerrainIsle = 97u;

    // 원본 함수에 넣은 실제 테두리 타입 번호 상수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private const uint TerrainEdge = 98u;

    // 원본 함수에 넣은 실제 프린지 타입 번호 상수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private const uint TerrainFringe = 102u;

    // 원본 함수에 넣은 실제 기타 타입 번호 상수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private const uint TerrainOther = 94u;

    // 팩토리가 반환하기 시작하는 공간 SID 번호 상수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private const ushort TerrainBorn = 40;

    // 사전 할당 공간의 마지막 SID 번호 상수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private const ushort TerrainEnd = 127;

    // 패치의 theme 0 초기 검색·직접 DWORD frame·지역/fringe 생성을 독립 원본과 비교하는 테스트 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    [Fact]
    public void TerrainStamp_ReplaysPatch()
    {
        ReplayTerrainStamp("originals");
    }

    // CD의 전체 후보·BYTE frame·실제 CRT cache/stamp를 독립 원본과 비교하는 테스트 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    [Fact]
    public void TerrainStamp_ReplaysCd()
    {
        ReplayTerrainStamp("originalCD");
    }

    // 같은 모듈에서 캐시가 재사용될 때 CRT가 다시 소비되지 않으며 미연결 하위 효과를 거부하는지 검증하는 테스트 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    [Fact]
    public void TerrainStamp_PreservesCacheAndRejectsMissingEffects()
    {
        string[][] rows = StampRows();
        TerrainStampScene scene = new(OriginalEdition.Patch1078);
        scene.Prepare(rows[0]);
        _ = scene.Stamp.FrameAt(50, 53);
        uint crt = scene.CrtState;
        int[] cache = (int[])scene.State.Variants!.Clone();
        _ = scene.Stamp.FrameAt(50, 53);
        Assert.Equal(crt, scene.CrtState);
        Assert.Equal(cache, scene.State.Variants);
        Assert.Throws<ArgumentException>(() => new RawTerrainStamp(
            scene.Pool,
            scene.Types,
            scene.Frames,
            scene.Map,
            scene.Random,
            () => scene.CrtState,
            v => scene.CrtState = v,
            scene.State,
            new TerrainStampHooks()));
    }

    // 원본이 읽는 코드 저장 순서를 구성하는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static RiftTypeFrames TerrainCodes(uint dataset, bool fringe)
    {
        List<RiftFrameCode> codes = new();
        if (dataset == 1u || (dataset == 4u && fringe))
        {
            return new RiftTypeFrames(Array.Empty<RiftFrameCode>());
        }

        if (dataset == 4u)
        {
            dataset = 0u;
        }

        if (dataset == 2u)
        {
            // 최대 256 후보를 넘는 동일 code 300개를 원본과 같은 순서로 추가하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            for (uint index = 0u; index < 300u; ++index)
            {
                codes.Add(new RiftFrameCode((byte)'A', (byte)'A', unchecked((byte)index), 0));
            }

            return new RiftTypeFrames(codes);
        }

        static void Append(List<RiftFrameCode> list, bool isFringe, byte side, byte variant)
        {
            ReadOnlySpan<byte> flags = stackalloc byte[] { 0, 2, 0x80, 1 };
            // 같은 code의 8개 후보를 추가하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            for (uint index = 0u; index < 8u; ++index)
            {
                list.Add(new RiftFrameCode(side, variant, unchecked((byte)index), isFringe ? flags[(int)(index % 4u)] : (byte)1));
            }
        }

        if (dataset == 0u)
        {
            // 물리 코드의 side 외부 순서를 구성하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            for (byte side = (byte)'A'; side <= (byte)'P'; ++side)
            {
                // 물리 코드의 variant 내부 순서를 구성하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
                for (byte variant = (byte)'A'; variant <= (byte)'P'; ++variant)
                {
                    Append(codes, fringe, side, variant);
                }
            }
        }
        else
        {
            ReadOnlySpan<byte> sparseSides = stackalloc byte[] { (byte)'A', (byte)'C', (byte)'F', (byte)'J', (byte)'P' };
            // 희소 코드의 원본 입력 순서를 추가하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            for (int i = 0; i < sparseSides.Length; ++i)
            {
                Append(codes, fringe, sparseSides[i], (byte)'A');
            }
        }

        return new RiftTypeFrames(codes);
    }

    // 현재 픽스처를 결과 재계산 없이 읽는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static string[][] StampRows()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "terrainstamp-x86.tsv");
        string[] lines = File.ReadAllLines(path);
        List<string[]> rows = new(lines.Length);
        // 헤더 이후의 모든 데이터 행을 탭 기준으로 분할하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (int i = 1; i < lines.Length; ++i)
        {
            if (!string.IsNullOrWhiteSpace(lines[i]))
            {
                rows.Add(lines[i].Split('\t'));
            }
        }

        return rows.ToArray();
    }

    // 지정 폭만큼 리틀엔디안 부호 없는 정수를 읽는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static uint GetField(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        uint value = 0u;
        // 지정 바이트 폭을 하위 바이트부터 결합하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (int index = 0; index < width; ++index)
        {
            value |= (uint)bytes[offset + index] << (index * 8);
        }

        return value;
    }

    // 실제 프레임·두 난수·99 캐시·현재 raw 생성·매핑·외부 효과 순서를 전수 대조하는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static void ReplayTerrainStamp(string edition)
    {
        TerrainStampScene scene = new(edition == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
        int count = 0;
        // 저장된 네이티브 결과와 생산 모듈의 현재 결과만 비교하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        foreach (string[] row in StampRows())
        {
            if (row[0] != edition)
            {
                continue;
            }

            Assert.Equal(12, row.Length);
            scene.Prepare(row);
            uint result = 0u;
            if (scene.Kind == "Frame")
            {
                result = unchecked((uint)scene.Stamp.FrameAt(unchecked((int)scene.Input[3]), unchecked((int)scene.Input[4])));
            }
            else
            {
                scene.Stamp.Stamp((sbyte)'x');
            }

            string cache = scene.State.Variants is not null
                ? string.Join(',', scene.State.Variants)
                : "-";
            string effects = scene.Events.Count > 0
                ? string.Join(';', scene.Events)
                : "-";
            StringBuilder raw = new();
            // 생성한 모든 raw 슬롯의 type/frame/owner/region/좌표를 실제 폭으로 읽는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            for (ushort sid = TerrainBorn; sid < scene.Born; ++sid)
            {
                if (raw.Length > 0)
                {
                    raw.Append(';');
                }

                ReadOnlySpan<byte> slot = scene.Pool.Slot(sid);
                raw.Append(sid.ToString(CultureInfo.InvariantCulture)).Append(':')
                    .Append(slot[10].ToString(CultureInfo.InvariantCulture)).Append(':')
                    .Append(GetField(slot, scene.Patch ? 36 : 34, scene.Patch ? 4 : 1).ToString(CultureInfo.InvariantCulture)).Append(':')
                    .Append(slot[scene.Patch ? 34 : 32].ToString(CultureInfo.InvariantCulture)).Append(':')
                    .Append(GetField(slot, 8, 2).ToString(CultureInfo.InvariantCulture)).Append(':')
                    .Append(GetField(slot, 14).ToString(CultureInfo.InvariantCulture)).Append(':')
                    .Append(GetField(slot, 18).ToString(CultureInfo.InvariantCulture));
            }

            string rawText = raw.Length == 0 ? "-" : raw.ToString();
            string mapping = $"{scene.Mapping['x']},{scene.Mapping['X']},{scene.Mapping['=']}";
            Assert.Equal(uint.Parse(row[3], CultureInfo.InvariantCulture), result);
            Assert.Equal(uint.Parse(row[4], CultureInfo.InvariantCulture), scene.Random.State());
            Assert.Equal(uint.Parse(row[5], CultureInfo.InvariantCulture), scene.CrtState);
            Assert.Equal(row[6], cache);
            Assert.Equal(row[7], effects);
            Assert.Equal(row[8], rawText);
            Assert.Equal(uint.Parse(row[9], CultureInfo.InvariantCulture), unchecked((uint)scene.Map.LastSid));
            Assert.Equal(row[10], mapping);
            Assert.Equal(uint.Parse(row[11], CultureInfo.InvariantCulture), scene.Assertions);
            ++count;
        }

        Assert.Equal(780, count);
    }

    // 원본의 현재 지도·두 난수·전역 캐시와 단일 raw 슬롯을 조립하는 장면 클래스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private sealed class TerrainStampScene
    {
        // 같은 factory/owner/Pop/region의 raw 저장소 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public RawSidPool Pool { get; }

        // 현재 genus 표의 입력 배열 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public OriginalTypeRecord[] Types { get; }

        // 실제 코드/J 첫 프레임 입력 배열 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public RiftTypeFrames[] Frames { get; }

        // 원본과 같은 X열 우선 문자 지도 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public byte[] Codes { get; } = new byte[65536];

        // 원본과 같은 X열 우선 지역 지도 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public byte[] Regions { get; } = new byte[65536];

        // 문자별 타입 매핑 배열 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public int[] Mapping { get; } = new int[256];

        // 문자별 대응 문자 배열 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public int[] Aliases { get; } = new int[256];

        // 실제 지도 배열과 현재 컨텍스트를 공유하는 맵 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public TerrainStampMap Map { get; }

        // 초기 stamp만 쓰는 전역 99 캐시 상태 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public TerrainStampState State { get; private set; } = new();

        // 실제 코드/fringe 후보가 소비하는 난수기 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public GameRandom Random { get; } = new();

        // 캐시 생성의 독립 CRT holdrand 상태 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public uint CrtState { get; set; }

        // 원본 픽스처의 입력 리스트 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public List<uint> Input { get; } = new();

        // 외부 팩토리/가상 효과의 실제 인자/순서 리스트 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public List<string> Events { get; } = new();

        // 현재 팩토리가 반환할 슬롯 번호 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public ushort Born { get; private set; }

        // 실제 assertion 경계의 호출 수 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public uint Assertions { get; private set; }

        // raw frame/owner의 현재 판본 배치 여부 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public bool Patch { get; }

        // Frame 또는 Stamp 진입점 문자열 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public string Kind { get; private set; } = string.Empty;

        // 단일 저장소를 쓰는 복원 모듈 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public RawTerrainStamp Stamp { get; private set; } = null!;

        // 명시 클라이언트 번호를 확보하고 결과 판단 전에 원본 입력만 준비하는 생성자다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public TerrainStampScene(OriginalEdition edition)
        {
            Pool = new RawSidPool(24000, false, null, edition);
            Types = new OriginalTypeRecord[edition == OriginalEdition.Patch1078 ? 188 : 171];
            // 타입 레코드 배열을 초기화하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            for (int i = 0; i < Types.Length; ++i)
            {
                Types[i] = new OriginalTypeRecord();
            }

            Frames = new RiftTypeFrames[Types.Length];
            // 프레임 배열을 빈 목록으로 초기화하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            for (int i = 0; i < Frames.Length; ++i)
            {
                Frames[i] = new RiftTypeFrames(Array.Empty<RiftFrameCode>());
            }

            Patch = edition == OriginalEdition.Patch1078;
            // 픽스처가 생성할 모든 SID를 사전 확보하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            for (ushort sid = 5; sid <= TerrainEnd; ++sid)
            {
                Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
            }

            Map = new TerrainStampMap
            {
                Codes = Codes,
                Regions = Regions,
                TypeMapping = Mapping,
                Aliases = Aliases,
            };
        }

        // 모든 입력을 원본과 같은 raw 지도/타입/캐시/두 난수로 구성하는 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public void Prepare(string[] row)
        {
            Kind = row[1];
            Input.Clear();
            Events.Clear();
            Assertions = 0u;
            Born = TerrainBorn;
            Array.Clear(Codes);
            Array.Clear(Regions);
            Array.Clear(Mapping);
            Array.Clear(Aliases);
            State = new TerrainStampState();
            Map.LastSid = 0;
            // 입력의 DWORD 비트를 그대로 읽는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            foreach (string number in row[2].Split(','))
            {
                Input.Add(number.StartsWith('-')
                    ? unchecked((uint)int.Parse(number, CultureInfo.InvariantCulture))
                    : uint.Parse(number, CultureInfo.InvariantCulture));
            }

            // 사전에 확보한 슬롯을 비우는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            for (ushort sid = 5; sid <= TerrainEnd; ++sid)
            {
                Pool.AllocatedBytes(sid).Clear();
            }

            uint dataset = Input[Kind == "Frame" ? 7 : 1];
            uint cache = Input[Kind == "Frame" ? 8 : 2];
            CrtState = Input[Kind == "Frame" ? 5 : 3];
            Random.SetState(Input[Kind == "Frame" ? 6 : 4]);
            if (cache != 0u)
            {
                State.Variants = new int[99];
                // 이미 존재하는 전역 캐시를 명시 입력으로 채우는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
                for (int index = 0; index < 99; ++index)
                {
                    State.Variants[index] = ((index * 17) + 3) % 32768;
                }
            }

            ReadOnlySpan<uint> configuredTypes = stackalloc uint[] { TerrainIsle, TerrainEdge, TerrainFringe, TerrainOther };
            // 독립 원본에 넣은 네 타입의 코드/genus를 같은 생산표에 전달하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            for (int i = 0; i < configuredTypes.Length; ++i)
            {
                uint type = configuredTypes[i];
                Types[type] = new OriginalTypeRecord { Flags2 = type == TerrainIsle ? 2u : 0u };
                Frames[type] = TerrainCodes(dataset, type == TerrainFringe);
            }

            Mapping['x'] = (int)TerrainIsle;
            Mapping['='] = (int)TerrainEdge;
            Mapping['?'] = (int)TerrainOther;
            Mapping['h'] = (int)TerrainIsle;
            Aliases['x'] = 'x';
            Aliases['='] = '=';
            Aliases['?'] = '?';
            Aliases['h'] = 'x';
            if (Kind == "Frame")
            {
                Assert.Equal(10, Input.Count);
                int x = unchecked((int)Input[3]);
                int y = unchecked((int)Input[4]);
                Map.Bounds[0] = 0;
                Map.Bounds[1] = 0;
                Map.Bounds[2] = 255;
                Map.Bounds[3] = 255;
                Map.MakePredictable = 1u;
                Map.ExcludedFlags = 0u;
                Cell(x, y, unchecked((byte)Input[0]));
                (int Dx, int Dy)[] neighbors = TerrainBuilder.NeighborCells;
                // 네이티브 입력과 같은 여덟 이웃 마스크/alias/지역 불일치만 넣는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
                for (int index = 0; index < neighbors.Length; ++index)
                {
                    (int dx, int dy) = neighbors[index];
                    uint mask = Input[(index % 2) != 0 ? 2 : 1];
                    Cell(
                        x + dx,
                        y + dy,
                        (mask & (1u << (index / 2))) != 0u ? (byte)'h' : (byte)'?',
                        (byte)(Input[9] != 0u ? 6 : 5));
                }
            }
            else
            {
                Assert.Equal(8, Input.Count);
                Map.Bounds[0] = 50;
                Map.Bounds[1] = 53;
                Map.Bounds[2] = 52;
                Map.Bounds[3] = 55;
                Map.MakePredictable = Input[6];
                Map.ExcludedFlags = Input[5];
                Cell(50, 53, (byte)'x');
                Cell(51, 53, (byte)'x');
                Cell(50, 54, (byte)'x');
                Cell(51, 54, (byte)'x');
                if (Input[0] == 1u)
                {
                    Cell(51, 53, (byte)'?');
                }

                if (Input[0] == 2u)
                {
                    Cell(50, 53, (byte)'h');
                    Mapping['h'] = 0;
                }

                if (Input[0] == 3u)
                {
                    Cell(51, 54, (byte)'h', 6);
                }

                if (Input[0] == 4u)
                {
                    Map.Bounds[2] = 50;
                }
            }

            Stamp = new RawTerrainStamp(
                Pool,
                Types,
                Frames,
                Map,
                Random,
                () => CrtState,
                v => CrtState = v,
                State,
                BuildHooks());
        }

        // 지도 밖 이웃은 쓰지 않고 원본이 bounds 검사로 배제하게 두는 셀 설정 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        private void Cell(int x, int y, byte code, byte region = 5)
        {
            if ((uint)x >= 256u || (uint)y >= 256u)
            {
                return;
            }

            int index = (x * 256) + y;
            Codes[index] = code;
            Regions[index] = region;
        }

        // 필수 하위 경계의 동일 입력/생성 raw 효과를 설치하는 훅 생성 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        private TerrainStampHooks BuildHooks()
        {
            return new TerrainStampHooks
            {
                Create = (type, predictable) =>
                {
                    int sid = Born++;
                    Span<byte> raw = Pool.AllocatedBytes(sid);
                    raw.Clear();
                    raw[10] = unchecked((byte)type);
                    Events.Add($"C:{sid}:{type}:{predictable}");
                    return sid;
                },
                SetOwner = (sid, owner) =>
                {
                    Pool.AllocatedBytes(sid)[Patch ? 34 : 32] = unchecked((byte)owner);
                    Events.Add($"O:{sid}:{owner}");
                },
                Pop = (sid, x, y, flags) =>
                {
                    Span<byte> raw = Pool.AllocatedBytes(sid);
                    uint bx = BitConverter.SingleToUInt32Bits(x);
                    uint by = BitConverter.SingleToUInt32Bits(y);
                    BinaryPrimitives.WriteUInt32LittleEndian(raw.Slice(14, 4), bx);
                    BinaryPrimitives.WriteUInt32LittleEndian(raw.Slice(18, 4), by);
                    Events.Add($"P:{sid}:{bx}:{by}:{flags}");
                },
                SetRegion = (sid, region) =>
                {
                    Events.Add($"R:{sid}:{unchecked((uint)region)}");
                    RawSquidRegion.SetSquidRegion(
                        Pool,
                        Types,
                        sid,
                        unchecked((uint)region),
                        unchecked((uint)State.NoRegion),
                        _ =>
                        {
                            ++Assertions;
                            Events.Add("A");
                        });
                },
                ChunkRegion = (x, y) =>
                {
                    Events.Add($"H:{x}:{y}");
                    return 6;
                },
                Assertion = _ =>
                {
                    ++Assertions;
                    Events.Add("A");
                },
            };
        }
    }
}

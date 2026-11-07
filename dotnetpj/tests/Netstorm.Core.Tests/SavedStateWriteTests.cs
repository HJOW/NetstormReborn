// 일반 전투 !state.dat 쓰기의 관문·바이트 배치·진단·예외 닫기를 검증한다.
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// RawSavedState 쓰기 본체를 검증하는 테스트 클래스다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
public sealed class SavedStateWriteTests
{
    /// <summary>쓰기 장면을 만든다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    private sealed class WriteScene
    {
        /// <summary>현재 9×180바이트다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public byte[] Players = PlayerRecords();

        /// <summary>현재 이름 저장소다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public byte[] Handle = new byte[64];

        /// <summary>이전 이름 복사 대상이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public byte[] Previous = new byte[64];

        /// <summary>현재 Fort 이름이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public byte[] Fort = new byte[300];

        /// <summary>복원 owner 출력이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public uint RestoreOwner;

        /// <summary>복원 subscriber 출력이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public uint RestoreSubscriber;

        /// <summary>기록한 바이트다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public List<byte> Stream = [];

        /// <summary>진단 호출이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public List<string> Assertions = [];

        /// <summary>진단 횟수다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public int Diagnosed;

        /// <summary>열기 인자다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public List<string> Opens = [];

        /// <summary>닫기 횟수다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public int Closes;

        /// <summary>관문 값이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public bool Post = true;

        /// <summary>알려짐 값이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public bool Known = true;

        /// <summary>검사 값이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public bool CheckingValue = true;

        /// <summary>장면을 만든다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        public WriteScene()
        {
            Put(Handle, 0, (byte)'Y');
            Put(Handle, 1, (byte)'o');
            Put(Handle, 2, (byte)'u');
            Put(Fort, 0, (byte)'F');
            Put(Fort, 1, (byte)'o');
            Put(Fort, 2, (byte)'r');
            Put(Fort, 3, (byte)'t');
            Put(Fort, 4, (byte)'A');
        }

        /// <summary>한 바이트를 쓴다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        /// <param name="bytes">대상이다.</param>
        /// <param name="at">위치다.</param>
        /// <param name="value">값이다.</param>
        private static void Put(byte[] bytes, int at, byte value) => bytes[at] = value;

        /// <summary>현재 9기록을 만든다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        /// <returns>9×180바이트다.</returns>
        private static byte[] PlayerRecords()
        {
            var records = new byte[9 * 180];
            // 아홉 기록을 순서대로 채우는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
            for (int player = 0; player < 9; ++player)
            {
                Dword(records, player * 180 + 0x2c, 0x1000u + (uint)player);
                Dword(records, player * 180 + 0x50, 0x40u);
                Dword(records, player * 180 + 0x58, 0x2000u + (uint)player);
                // 두 30바이트 구간을 채우는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
                for (int index = 0; index < 30; ++index)
                {
                    records[player * 180 + 12 + index] = (byte)(player * 30 + index);
                    records[player * 180 + 0x30 + index] = (byte)(0x80 + player * 30 + index);
                }

                // 10바이트 구간을 채우는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
                for (int index = 0; index < 10; ++index)
                {
                    records[player * 180 + 0x6c + index] = (byte)(0xc0 + player * 10 + index);
                }
            }

            return records;
        }

        /// <summary>DWORD를 쓴다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        /// <param name="bytes">대상이다.</param>
        /// <param name="at">위치다.</param>
        /// <param name="value">값이다.</param>
        private static void Dword(byte[] bytes, int at, uint value)
        {
            // 낮은 바이트부터 쓰는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
            for (int index = 0; index < 4; ++index)
            {
                bytes[at + index] = (byte)(value >> (index * 8));
            }
        }

        /// <summary>전역을 만든다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        /// <returns>쓰기 전역이다.</returns>
        public SavedStateWriteGlobals Globals()
        {
            var scene = this;
            return new SavedStateWriteGlobals
            {
                MissionPost = () => !scene.Post,
                PlayerKnown = () => scene.Known,
                Checking = () => scene.CheckingValue,
                LocalOwner = () => 3u,
                MySubscriber = () => 0xabcdef01u,
                Handle = () => scene.Handle,
                Players = () => scene.Players,
                SetRestoreOwner = value => scene.RestoreOwner = value,
                SetRestoreSubscriber = value => scene.RestoreSubscriber = value,
                PreviousHandle = scene.Previous,
                Tail = [() => 0x11111111u, () => 0x22222222u, () => 0x33333333u],
                LoginEnd = () => 1.5,
            };
        }

        /// <summary>훅을 만든다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
        /// <returns>쓰기 훅이다.</returns>
        public SavedStateWriteHooks Hooks()
        {
            var scene = this;
            return new SavedStateWriteHooks
            {
                CurrentFort = () => scene.Fort,
                Diagnose = () => ++scene.Diagnosed,
                Open = (first, second) => scene.Opens.Add($"{first}:{second}"),
                Write = (bytes, offset) =>
                {
                    Assert.Equal(-1, offset);
                    scene.Stream.AddRange(bytes);
                },
                Close = () => ++scene.Closes,
                WallSeconds = () => 100.7,
                LocalSp = () => 3.5f,
                Assertion = (text, code) => scene.Assertions.Add($"{text}:{code}"),
            };
        }
    }

    /// <summary>DWORD를 읽는다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="at">위치다.</param>
    /// <returns>조합한 값이다.</returns>
    private static uint Get(IReadOnlyList<byte> bytes, int at)
    {
        uint value = 0u;
        // 낮은 바이트부터 모으는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
        for (int i = 0; i < 4; i++)
        {
            value |= (uint)bytes[at + i] << (i * 8);
        }

        return value;
    }

    /// <summary>post 또는 미known이면 장치 없이 반환한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    [Fact]
    public void GatesReturnWithoutDevices()
    {
        var scene = new WriteScene();
        scene.Post = false;
        RawSavedState.WriteSavedState(scene.Globals(), scene.Hooks());
        Assert.Equal(0, scene.Diagnosed);
        Assert.Empty(scene.Opens);
        Assert.Empty(scene.Stream);
        var unknown = new WriteScene { Known = false };
        RawSavedState.WriteSavedState(unknown.Globals(), unknown.Hooks());
        Assert.Equal(0, unknown.Diagnosed);
        Assert.Empty(unknown.Opens);
    }

    /// <summary>서명·이름·owner·시각·8행·꼬리·종료 순서를 쓴다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    [Fact]
    public void FullWriteLayout()
    {
        var scene = new WriteScene();
        RawSavedState.WriteSavedState(scene.Globals(), scene.Hooks());
        Assert.Equal(["2:1"], scene.Opens);
        Assert.Equal(1, scene.Diagnosed);
        Assert.Equal(1, scene.Closes);
        Assert.Empty(scene.Assertions);
        byte[] stream = [.. scene.Stream];
        Assert.Equal(976, stream.Length);
        Assert.Equal(0xa004eu, Get(stream, 0));
        Assert.Equal([(byte)'F', (byte)'o', (byte)'r', (byte)'t', (byte)'A', 0], stream.AsSpan(4, 6).ToArray());
        Assert.Equal(0u, Get(stream, 256));
        Assert.Equal(3u, Get(stream, 260));
        Assert.Equal(100u, Get(stream, 264));
        Assert.Equal(3u, Get(stream, 268));
        // 여덟 행을 순서대로 대조하는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
        for (int row = 0; row < 8; ++row)
        {
            int at = 272 + row * 86;
            int player = row + 1;
            Assert.Equal((uint)player, Get(stream, at));
            Assert.Equal(0x1000u + (uint)player, Get(stream, at + 4));
            // 두 30바이트 구간을 대조하는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
            for (int index = 0; index < 30; ++index)
            {
                Assert.Equal((byte)(player * 30 + index), stream[at + 8 + index]);
                Assert.Equal((byte)(0x80 + player * 30 + index), stream[at + 38 + index]);
            }

            Assert.Equal(0x40u, Get(stream, at + 68));
            Assert.Equal(0x2000u + (uint)player, Get(stream, at + 72));
            // 10바이트 구간을 대조하는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
            for (int index = 0; index < 10; ++index)
            {
                Assert.Equal((byte)(0xc0 + player * 10 + index), stream[at + 76 + index]);
            }
        }

        Assert.Equal(0x11111111u, Get(stream, 960));
        Assert.Equal(0x22222222u, Get(stream, 964));
        Assert.Equal(0x33333333u, Get(stream, 968));
        Assert.Equal(1u, Get(stream, 972));
        Assert.Equal(3u, scene.RestoreOwner);
        Assert.Equal(0xabcdef01u, scene.RestoreSubscriber);
        Assert.Equal([(byte)'Y', (byte)'o', (byte)'u', 0], scene.Previous.AsSpan(0, 4).ToArray());
    }

    /// <summary>점에 진단하고 계속 쓴다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    [Fact]
    public void CheckingDotAssertion()
    {
        var scene = new WriteScene();
        scene.Fort[0] = (byte)'H';
        scene.Fort[1] = (byte)'a';
        scene.Fort[2] = (byte)'s';
        scene.Fort[3] = (byte)'.';
        scene.Fort[4] = 0;
        RawSavedState.WriteSavedState(scene.Globals(), scene.Hooks());
        Assert.Equal(["!(strchr(curFort,'.')):3961"], scene.Assertions);
        Assert.Equal(976, scene.Stream.Count);
        Assert.Equal(1, scene.Closes);
    }

    /// <summary>서버 둘 이상이면 진단한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    [Fact]
    public void ServerCountAssertion()
    {
        var scene = new WriteScene();
        scene.Players[2 * 180 + 0x50] |= 8;
        scene.Players[3 * 180 + 0x50] |= 8;
        RawSavedState.WriteSavedState(scene.Globals(), scene.Hooks());
        Assert.Equal(["serverCount <= 1:4003"], scene.Assertions);
        Assert.Equal(976, scene.Stream.Count);
    }

    /// <summary>쓰기 예외에서 파일을 한 번 닫고 부분 효과를 남긴다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    [Fact]
    public void ExceptionClosesOnceAndKeepsPartialEffects()
    {
        var scene = new WriteScene();
        var hooks = scene.Hooks();
        int calls = 0;
        hooks.Write = (_, _) =>
        {
            if (++calls == 5)
            {
                throw new InvalidOperationException("쓰기 실패");
            }

            scene.Stream.Add(0);
        };
        Assert.Throws<InvalidOperationException>(() => RawSavedState.WriteSavedState(scene.Globals(), hooks));
        Assert.Equal(1, scene.Closes);
        Assert.Equal(3u, scene.RestoreOwner);
    }

    /// <summary>비전투·신선 기한은 건너뛰고 만료면 저장 뒤 예약한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    [Fact]
    public void PollSkipsAndRearms()
    {
        double deadline = 100.0;
        int saves = 0;
        RawSavedState.PollSavedStateWrite(false, ref deadline, () => 999.0, () => ++saves);
        Assert.Equal(0, saves);
        Assert.Equal(100.0, deadline);
        RawSavedState.PollSavedStateWrite(true, ref deadline, () => 10.0, () => ++saves);
        Assert.Equal(0, saves);
        var times = new Queue<double>([50.0, 60.0]);
        deadline = 40.0;
        RawSavedState.PollSavedStateWrite(true, ref deadline, () => times.Dequeue(), () => ++saves);
        Assert.Equal(1, saves);
        Assert.Equal(70.0, deadline);
    }

    /// <summary>NUL 없는 이름은 닫고 거절한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    [Fact]
    public void MissingNulNameClosesAndRejects()
    {
        var scene = new WriteScene();
        Array.Fill(scene.Fort, (byte)'A');
        Assert.Throws<ArgumentOutOfRangeException>(() => RawSavedState.WriteSavedState(scene.Globals(), scene.Hooks()));
        Assert.Equal(1, scene.Closes);
    }

    /// <summary>작은 복사 대상은 닫고 거절한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    [Fact]
    public void SmallCopyTargetClosesAndRejects()
    {
        var scene = new WriteScene();
        scene.Previous = new byte[2];
        Assert.Throws<ArgumentOutOfRangeException>(() => RawSavedState.WriteSavedState(scene.Globals(), scene.Hooks()));
        Assert.Equal(1, scene.Closes);
    }

    /// <summary>9기록이 아니면 닫고 거절한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    [Fact]
    public void PlayersSizeMismatchClosesAndRejects()
    {
        var scene = new WriteScene();
        scene.Players = new byte[100];
        Assert.Throws<ArgumentOutOfRangeException>(() => RawSavedState.WriteSavedState(scene.Globals(), scene.Hooks()));
        Assert.Equal(1, scene.Closes);
    }
}

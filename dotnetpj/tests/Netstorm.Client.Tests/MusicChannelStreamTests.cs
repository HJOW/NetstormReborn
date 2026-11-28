using System.Globalization;
using Netstorm.Client.Audio;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 원문 음악 스트리밍5088행을 실제 채널·두 쓰기 구간·공개 갱신 부모와 전수 대조한다.
namespace Netstorm.Client.Tests;

/// <summary>파일/COM의 명시 응답만 공급하고 실제 FillBuffer/Read/Stop의 상태·출력·사건을 검증한다.</summary>
public sealed class MusicChannelStreamTests
{
    /// <summary>원본 관측에 공급한 실제 대체 버퍼 토큰이다.</summary>
    private const uint BufferToken = 0x16001010;
    /// <summary>원본 관측에 공급한 실제 대체 파일 토큰이다.</summary>
    private const uint FileToken = 0x17001000;
    /// <summary>원본 실행기에 공급한 E_FAIL HRESULT다.</summary>
    private const int Failure = unchecked((int)0x80004005);
    /// <summary>원문 숫자를 DWORD 비트 그대로 읽는다.</summary>
    private static uint Number(string text) => unchecked((uint)long.Parse(text, CultureInfo.InvariantCulture));
    /// <summary>원문 HRESULT·음수 IO 입력을 signed32로 읽는다.</summary>
    private static int Whole(string text) => unchecked((int)Number(text));
    /// <summary>원문 BYTE 배열 표기를 만든다.</summary>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexString(bytes).ToLowerInvariant();

    /// <summary>원문 COM/IO 입력과 감시 출력만 소유한다. 채우기·원시 채널 상태 전이는 대신 구현하지 않는다.</summary>
    private sealed class Device
    {
        /// <summary>명시 메모리 파일의 실제 바이트다.</summary>
        public byte[] File = [];
        /// <summary>서로 떨어진 첫/두 번째 쓰기 구간과 나머지 감시 바이트다.</summary>
        public byte[] Output = Enumerable.Repeat((byte)0xa7, 128).ToArray();
        /// <summary>현재 파일의 절대 위치다.</summary>
        public long Position = 5;
        /// <summary>현재 파일을 읽을 수 있는지 나타낸다.</summary>
        public bool Opened = true;
        /// <summary>현재 COM Lock이 반납되지 않았는지 나타낸다.</summary>
        public bool Locked;
        /// <summary>호스트 IO 예외 입력이다.</summary>
        public bool ReadThrows;
        /// <summary>잘못된 COM 구간 합계의 호스트 방어 검사 입력이다.</summary>
        public bool BadRegions;
        /// <summary>-2는 제한 없음,-1은 실패,그 밖은 부분 읽기 상한이다.</summary>
        public int Cap = -2;
        /// <summary>-1이면 실제 파일 되감기를 실패시킨다.</summary>
        public int SeekFailure;
        /// <summary>1~7은 상태/복구/음량/커서/Lock/Unlock/Play 중 실패할 COM 위치다.</summary>
        public int Failed;
        /// <summary>-1은 첫 구간 전체,그 밖은 두 구간의 첫 길이 상한이다.</summary>
        public int Split = -1;
        /// <summary>원문 재귀 잠금 관측의 현재 깊이다.</summary>
        public int LockDepth;
        /// <summary>현재 장치 상태 비트 입력이다.</summary>
        public uint Status;
        /// <summary>현재 장치 재생 커서 입력이다.</summary>
        public uint Cursor;
        /// <summary>실제 Create 경계가 반환할 HRESULT다.</summary>
        public int CreateResult;
        /// <summary>실제 Create 경계가 비영 토큰 또는0을 쓸지 선택한다.</summary>
        public bool CreateWrites = true;
        /// <summary>이번 명령에서 일어난 순서 있는 IO/COM/잠금/기록 사건이다.</summary>
        private readonly List<string> _events = [];

        /// <summary>실행기와 같은 독립 파일 입력 패턴과 위치를 준비한다.</summary>
        public void SetFile(uint length, uint offset)
        {
            File = new byte[checked(5 + (int)length)];
            Position = 5 + offset;
            // 결과가 아닌 실제 입력 표본만 순서대로 배치한다.
            for (int index = 0; index < File.Length; index++) File[index] = unchecked((byte)(index * 7 + 11));
        }

        /// <summary>같은 명시 파일·버퍼 경계를 실제 채널 제어에 공급한다.</summary>
        public MusicChannelHooks ChannelHooks() => new()
        {
            SetVolume = (token, volume) => { Assert.Equal(BufferToken, token); _events.Add($"V:#1:{volume}"); return Failed == 3 ? Failure : 0; },
            Stop = token => { Assert.Equal(BufferToken, token); _events.Add("X:#1"); },
            Release = token => { Assert.Equal(BufferToken, token); _events.Add("B:#1"); return 0; },
            Seek = (token, absolute) =>
            {
                int result = Opened && token == FileToken && SeekFailure != -1 ? unchecked((int)absolute) : -1;
                _events.Add($"S:{token}:{absolute}:{result}");
                if (result != -1) Position = absolute;
                return result;
            },
            Read = (token, target) =>
            {
                if (ReadThrows) throw new IOException("음악 읽기 예외 입력");
                long before = Position;
                int result = -1;
                if (Opened && token == FileToken && Cap != -1)
                {
                    int size = Math.Min(target.Length, Position > File.Length ? 0 : File.Length - checked((int)Position));
                    if (Cap >= 0) size = Math.Min(size, Cap);
                    if (size != 0) File.AsSpan((int)Position, size).CopyTo(target);
                    Position += size;
                    result = size;
                }
                _events.Add($"R:{token}:{before}:{target.Length}:{result}");
                return result;
            },
            Close = token => { _events.Add($"C:{token}"); Opened = false; },
            Log = (channel, text) => _events.Add($"G:{channel}:{text.TrimEnd('\n')}"),
            LockEvent = entering => { LockDepth += entering ? 1 : -1; Assert.True(LockDepth >= 0); _events.Add(entering ? "E" : "L"); },
        };

        /// <summary>상태 판단 없이 명시 COM 반환/두 쓰기 구간만 제공한다.</summary>
        public MusicBufferHooks BufferHooks() => new()
        {
            Create = (ReadOnlySpan<byte> format, uint flags, uint bytes, ref uint token) =>
            {
                token = CreateWrites ? BufferToken : 0;
                _events.Add($"N:{flags}:{bytes}:{Hex(format)}:{token}:{CreateResult}");
                return CreateResult;
            },
            Status = (uint token, out uint status) =>
            {
                Assert.Equal(BufferToken, token); status = Status;
                int result = Failed == 1 ? Failure : 0;
                _events.Add($"T:#1:{status}:{result}");
                return result;
            },
            Restore = token => { Assert.Equal(BufferToken, token); int result = Failed == 2 ? Failure : 0; _events.Add($"A:#1:{result}"); return result; },
            Cursor = (uint token, out uint play, out uint write) =>
            {
                Assert.Equal(BufferToken, token); play = Cursor; write = 13;
                int result = Failed == 4 ? Failure : 0;
                _events.Add($"Q:#1:{play}:13:{result}");
                return result;
            },
            Lock = (uint token, uint offset, uint count, ref MusicBufferRegions regions) =>
            {
                Assert.Equal(BufferToken, token); Assert.False(Locked);
                int result = Failed == 5 ? Failure : 0;
                uint first = result < 0 ? 0 : Split < 0 ? count : Math.Min(count, (uint)Split);
                uint second = result < 0 ? 0 : count - first;
                if (result >= 0)
                {
                    regions = new(Output.AsSpan(0, (int)first), Output.AsSpan(64, (int)second + (BadRegions ? 1 : 0)));
                    Locked = true;
                }
                _events.Add($"K:#1:{offset}:{count}:{first}:{second}:{result}");
                return result;
            },
            Unlock = (token, regions) =>
            {
                Assert.Equal(BufferToken, token); Assert.True(Locked); Locked = false;
                int result = Failed == 6 ? Failure : 0;
                _events.Add($"U:#1:{regions.First.Length}:{regions.Second.Length}:{result}");
                return result;
            },
            Play = (token, flags) => { Assert.Equal(BufferToken, token); int result = Failed == 7 ? Failure : 0; _events.Add($"P:#1:{flags}:{result}"); return result; },
        };

        /// <summary>사건 순서를 바꾸지 않고 원문 관측 표기와 연결한다.</summary>
        public string Events() => _events.Count == 0 ? "-" : string.Join(';', _events);
        /// <summary>다음 독립 명령용 사건만 비운다.</summary>
        public void ClearEvents() => _events.Clear();
    }

    /// <summary>같은 전역·실제 채널·명시 IO와 공개 음악 부모를 결합한다.</summary>
    private sealed class Scene
    {
        /// <summary>공개 음악과 FillBuffer가 공유하는 현재 전역이다.</summary>
        public SoundState Sound { get; } = new() { MusicInitialized = true, MusicVolume = -777 };
        /// <summary>명시 COM/파일 응답과 출력이다.</summary>
        public Device Device { get; } = new();
        /// <summary>같은 원문96BYTE 채널 상태다.</summary>
        public MusicChannelState State { get; } = new();
        /// <summary>실제 읽기/채우기/정지 본체다.</summary>
        public MusicChannel Channel { get; }
        /// <summary>같은 명시 primary identity와 명령 잠금을 소유한 공개 갱신 부모다.</summary>
        public SoundMusicControl Music { get; }

        /// <summary>효과음은 이번 입력에서 선택되지 않으며 호출되면 명시 실패한다.</summary>
        public Scene(OriginalEdition edition = OriginalEdition.Patch1078)
        {
            var list = new SoundList(edition);
            var effects = new SoundPlayer(list, Sound, new()
            {
                Status = _ => throw new InvalidOperationException("예상하지 않은 효과음 조회"),
                Play = (_, _) => throw new InvalidOperationException("예상하지 않은 효과음 재생"),
                SetPosition = (_, _) => throw new InvalidOperationException("예상하지 않은 효과음 위치"),
                SetVolume = (_, _) => throw new InvalidOperationException("예상하지 않은 효과음 음량"),
                SetPan = (_, _) => throw new InvalidOperationException("예상하지 않은 효과음 좌우"),
                Stop = _ => throw new InvalidOperationException("예상하지 않은 효과음 정지"),
                Duplicate = (uint _, ref uint copy) => throw new InvalidOperationException("예상하지 않은 효과음 복제"),
                Load = _ => throw new InvalidOperationException("예상하지 않은 효과음 적재"),
            });
            Channel = new(edition, State, Device.ChannelHooks());
            Music = SoundMusicControl.ForPrimary(edition, Sound, effects, Channel);
            Seed();
        }

        /// <summary>관측 실행기와 동일한 입력 바이트를 쓰고 기대 출력으로 상태를 초기화하지 않는다.</summary>
        public void Seed()
        {
            Span<byte> raw = State.Raw().Span;
            // 미작성 패딩·잠금 공간을 포함한96BYTE 전수 보존을 관찰하는 입력이다.
            for (int index = 0; index < raw.Length; index++) raw[index] = unchecked((byte)(index * 17 + 3));
            State.SetField(MusicField.Buffer, BufferToken);
            State.SetField(MusicField.BufferBytes, 16);
            State.SetField(MusicField.Flags, 3);
            State.SetField(MusicField.File, FileToken);
            State.SetField(MusicField.Length, 16);
            State.SetField(MusicField.ReadOffset, 0);
            State.SetField(MusicField.WriteOffset, 9);
            State.SetField(MusicField.DataOffset, 5);
            State.SetField(MusicField.StopDepth, 0);
            Device.SetFile(16, 0);
        }
    }

    /// <summary>세 실제 PE에서 독립 관측한5088행의 사건·raw96·두 출력·파일 위치/열림을 전수 대조한다.</summary>
    [Theory]
    [InlineData("originals", OriginalEdition.Patch1078, 1704)]
    [InlineData("originalCD", OriginalEdition.Cd1072, 1692)]
    [InlineData("original1037", OriginalEdition.Cd1072, 1692)]
    public void ActualStreamMatchesAllNativeObservations(string editionName, OriginalEdition edition, int expected)
    {
        string[][] all = X86Fixture.Read("musicstream");
        Assert.Equal(5088, all.Length);
        string[][] rows = all.Where(row => row[0] == editionName).ToArray();
        Assert.Equal(expected, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(8, row.Length);
            var scene = new Scene(edition);
            string[] fields = row[1][2..].Split(',');
            bool fill = row[1][0] == 'F';
            if (fill)
            {
                scene.State.SetField(MusicField.Flags, Number(fields[0]));
                scene.Sound.MusicInitialized = Number(fields[1]) != 0;
                scene.Device.Status = Number(fields[2]);
                scene.Device.Cursor = Number(fields[3]);
                scene.State.SetField(MusicField.WriteOffset, Number(fields[4]));
                scene.State.SetField(MusicField.Length, Number(fields[5]));
                scene.State.SetField(MusicField.ReadOffset, Number(fields[6]));
                scene.Device.SetFile(Number(fields[5]), Number(fields[6]));
                scene.Device.Cap = Whole(fields[7]);
                scene.Device.SeekFailure = Whole(fields[8]);
                scene.Device.Failed = Whole(fields[9]);
                scene.Device.Split = Whole(fields[10]);
            }
            else
            {
                scene.State.SetField(MusicField.Buffer, Number(fields[0]) != 0 ? BufferToken : 0);
                scene.Device.CreateResult = Whole(fields[1]);
                scene.Device.CreateWrites = Number(fields[2]) != 0;
            }
            MusicBufferHooks hooks = scene.Device.BufferHooks();
            bool result = fill
                ? edition == OriginalEdition.Patch1078 ? scene.Channel.FillBuffer(hooks, scene.Sound) : scene.Music.Update(hooks)
                : scene.Channel.EnsureBuffer(hooks);
            Assert.Equal(0, scene.Device.LockDepth);
            Assert.False(scene.Device.Locked);
            Assert.Equal(row[2], scene.Device.Events());
            if (edition == OriginalEdition.Patch1078) Assert.Equal(row[3], result ? "1" : "0");
            else Assert.Equal("-", row[3]);
            Assert.Equal(Convert.FromHexString(row[4]), scene.State.Raw().ToArray());
            Assert.Equal(Convert.FromHexString(row[5]), scene.Device.Output);
            Assert.Equal((long)Number(row[6]), scene.Device.Position);
            Assert.Equal(row[7], scene.Device.Opened ? "1" : "0");
            return true;
        });
    }

    /// <summary>실제 시작·첫 채우기·부분 갱신·두 구간 래핑·짧은 읽기 뒤 Unlock/Stop을 연결한다.</summary>
    [Fact]
    public void ActualStartFillWrapAndShortReadUseSameChannelAndOutput()
    {
        var scene = new Scene();
        scene.State.SetField(MusicField.Length, 64);
        scene.Device.SetFile(64, 0);
        Assert.True(scene.Channel.Start(1));
        Assert.True(scene.Music.Update(scene.Device.BufferHooks()));
        Assert.Equal(0u, scene.State.Field(MusicField.WriteOffset));
        Assert.Equal(16u, scene.State.Field(MusicField.ReadOffset));
        Assert.Contains("P:#1:1:0", scene.Device.Events());
        scene.Device.ClearEvents();
        scene.Device.Status = 1;
        scene.Device.Cursor = 12;
        Assert.True(scene.Music.Update(scene.Device.BufferHooks()));
        Assert.Equal(12u, scene.State.Field(MusicField.WriteOffset));
        scene.Device.ClearEvents();
        scene.Device.Cursor = 3;
        scene.Device.Split = 4;
        Assert.True(scene.Music.Update(scene.Device.BufferHooks()));
        Assert.Equal(3u, scene.State.Field(MusicField.WriteOffset));
        Assert.Equal(35u, scene.State.Field(MusicField.ReadOffset));
        Assert.Equal(scene.Device.File[33], scene.Device.Output[0]);
        Assert.Equal(scene.Device.File[37], scene.Device.Output[64]);
        Assert.Contains("K:#1:12:7:4:3:0", scene.Device.Events());
        scene.Device.ClearEvents();
        scene.Device.Cap = 2;
        scene.Device.Cursor = 8;
        Assert.False(scene.Music.Update(scene.Device.BufferHooks()));
        Assert.False(scene.Device.Opened);
        Assert.False(scene.Device.Locked);
        Assert.Equal(0, scene.Device.LockDepth);
        Assert.Equal(0u, scene.State.Field(MusicField.Buffer));
        Assert.Equal(2u, scene.State.Field(MusicField.Flags));
        string events = scene.Device.Events();
        Assert.True(events.IndexOf("U:#1:", StringComparison.Ordinal) < events.IndexOf("X:#1", StringComparison.Ordinal));
    }

    /// <summary>읽기 예외·잘못된 두 구간·범위 밖 커서에서 실제 Lock/채널 잠금을 반납한다.</summary>
    [Fact]
    public void IoAndRegionFailuresReleaseBorrowedRegionsAndChannelLock()
    {
        var scene = new Scene();
        scene.Device.ReadThrows = true;
        Assert.Throws<IOException>(() => scene.Channel.FillBuffer(scene.Device.BufferHooks(), scene.Sound));
        Assert.False(scene.Device.Locked);
        Assert.Equal(0, scene.Device.LockDepth);
        scene.Device.ReadThrows = false;
        scene.Device.BadRegions = true;
        Assert.Throws<ArgumentOutOfRangeException>(() => scene.Channel.FillBuffer(scene.Device.BufferHooks(), scene.Sound));
        Assert.False(scene.Device.Locked);
        Assert.Equal(0, scene.Device.LockDepth);
        scene.Device.BadRegions = false;
        scene.State.SetField(MusicField.WriteOffset, 16);
        scene.Device.Status = 1;
        Assert.Throws<ArgumentOutOfRangeException>(() => scene.Channel.FillBuffer(scene.Device.BufferHooks(), scene.Sound));
        Assert.Equal(0, scene.Device.LockDepth);
    }

    /// <summary>미준비·비활성·종료중은 COM을 요구하지 않고 기존 버퍼 확보도 생성기를 요구하지 않는다.</summary>
    [Fact]
    public void UnselectedAndExistingBuffersKeepNativeGuards()
    {
        var scene = new Scene(OriginalEdition.Cd1072);
        scene.Sound.MusicInitialized = false;
        Assert.False(scene.Music.Update(null));
        Assert.Equal("-", scene.Device.Events());
        scene.Sound.MusicInitialized = true;
        scene.Music.Stopping = true;
        Assert.False(scene.Music.Update(null));
        Assert.Equal("-", scene.Device.Events());
        scene.Music.Stopping = false;
        scene.State.SetField(MusicField.Flags, 2);
        Assert.False(scene.Channel.FillBuffer(null, scene.Sound));
        Assert.Equal("E;L", scene.Device.Events());
        scene.Device.ClearEvents();
        Assert.True(scene.Channel.EnsureBuffer(null));
        Assert.Equal(16u, scene.State.Field(MusicField.BufferBytes));
        Assert.Equal("E;L", scene.Device.Events());
        scene.State.SetField(MusicField.Flags, 3);
        Assert.Throws<ArgumentException>(() => scene.Channel.FillBuffer(null, scene.Sound));
        Assert.Equal(0, scene.Device.LockDepth);
    }
}

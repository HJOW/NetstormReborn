using System.Globalization;
using Netstorm.Client.Audio;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 독립 원문 음악 관측 중 공유 음소거 본체의396행을 실제 효과음 계층과 대조한다.
namespace Netstorm.Client.Tests;

/// <summary>음악 채널 전체를 구현하지 않고 SetVolume 경계만 명시 공급해 실제 부모의 순서·재진입·부분 쓰기를 검증한다.</summary>
public sealed class SoundMusicControlTests
{
    /// <summary>독립 관측의 효과음/음악 버퍼 토큰 기준이다.</summary>
    private const uint BufferBase = 0x16001000;

    /// <summary>원문DWORD 십진수의 signed 비트를 보존한다.</summary>
    private static int Whole(string text) => unchecked((int)uint.Parse(text, CultureInfo.InvariantCulture));

    /// <summary>실제 같은 SoundState/이름표/효과음 재생기와 명시된 음악 음량 경계를 조립한다.</summary>
    private sealed class Scene
    {
        /// <summary>음소거와 효과음이 공유하는 실제 현재 전역이다.</summary>
        public readonly SoundState State = new()
        {
            Initialized = true, Device = true, Enabled = true, MusicInitialized = true,
            MasterVolume = -400, PendingMasterVolume = 123, MusicVolume = -600, PendingMusicVolume = 222,
        };
        /// <summary>실제 원문 소리 표이며 최종 전체 Adler를 대조한다.</summary>
        public readonly SoundList List;
        /// <summary>같은 실제 소리 표와 전역을 사용하는 효과음 본체다.</summary>
        public readonly SoundPlayer Effects;
        /// <summary>검사 대상 공유 음소거 본체다.</summary>
        public readonly SoundMusicControl Control;
        /// <summary>명시 장치 호출과 재진입 효과의 순서 기록이다.</summary>
        private readonly List<string> _events = [];
        /// <summary>음량 COM 호출의 명시 HRESULT다.</summary>
        public int GainResult;
        /// <summary>다음 해당 버퍼의 음량 호출에서 한 번 적용할 원문 관측 입력이다.</summary>
        public (bool Music, int Depth, int Pending)? Callback;

        /// <summary>재생/정지 등 범위 밖 장치 호출은 실패시키고 음량 경계만 설치한다.</summary>
        public Scene(OriginalEdition edition, bool bindMusic = true)
        {
            List = new SoundList(edition);
            var hooks = new SoundDeviceHooks
            {
                Status = _ => throw new InvalidOperationException("미선택 소리 상태"),
                Play = (_, _) => throw new InvalidOperationException("미선택 재생"),
                SetPosition = (_, _) => throw new InvalidOperationException("미선택 위치"),
                SetVolume = Gain,
                SetPan = (_, _) => throw new InvalidOperationException("미선택 좌우"),
                Stop = _ => throw new InvalidOperationException("미선택 정지"),
                Duplicate = (uint _, ref uint _) => throw new InvalidOperationException("미선택 복제"),
                Load = _ => throw new InvalidOperationException("미선택 파일 적재"),
            };
            Effects = new SoundPlayer(List, State, hooks);
            Control = new SoundMusicControl(edition, State, Effects, bindMusic ? MusicVolume : null);
        }

        /// <summary>버퍼 종류에 해당하는 실제 음량 호출을 기록하고 명시 재진입 입력을 한 번 적용한다.</summary>
        private int Gain(uint buffer, int volume)
        {
            _events.Add($"V:#{(buffer - BufferBase) / 16}:{volume}");
            if (Callback is { } callback && buffer == BufferBase + (callback.Music ? 16u : 0u))
            {
                Callback = null;
                State.VolumeHoldDepth = callback.Depth;
                State.PendingMusicVolume = callback.Pending;
                _events.Add($"J:{callback.Depth}:{callback.Pending}");
            }
            return GainResult;
        }

        /// <summary>실제 MusicChannel.SetVolume의 잠금·COM·실패로그만 명시 장치 경계로 관측한다. 채널 구현 통과로 세지 않는다.</summary>
        private void MusicVolume(int volume)
        {
            _events.Add("E");
            try
            {
                if (Gain(BufferBase + 16, volume) < 0) _events.Add("G:9:Failed to set music buffer volume.");
            }
            finally { _events.Add("L"); }
        }

        /// <summary>원문 seed 명령의 동일 효과음 버퍼·저장 음량·감쇠 입력을 설치한다.</summary>
        public void Seed()
        {
            List.Initialize();
            uint effect = List.Lookup("effect.wav");
            List.SetField(effect, SoundField.Buffer, BufferBase);
            List.SetField(effect, SoundField.Attenuation, 300);
            List.SetField(effect, SoundField.Volume, unchecked((uint)-200));
        }

        /// <summary>이번 연산의 실제 사건을 읽고 다음 연산용 기록만 비운다.</summary>
        public string Take()
        {
            string text = _events.Count == 0 ? "-" : string.Join(';', _events);
            _events.Clear();
            return text;
        }
    }

    /// <summary>공유 음량 명령이 실제 존재하는132행씩만 선택한다. 전체 음악 제어5292행으로 확대하지 않는다.</summary>
    [Theory]
    [InlineData("originals", OriginalEdition.Patch1078)]
    [InlineData("originalCD", OriginalEdition.Cd1072)]
    [InlineData("original1037", OriginalEdition.Cd1072)]
    public void OriginalMuteSubsetMatchesCurrentStateAndEffects(string edition, OriginalEdition kind)
    {
        string[][] all = X86Fixture.Read("musiccontrol");
        Assert.Equal(5292, all.Length);
        string[][] rows = all.Where(row => row[0] == edition && row[1].Split('|').Any(operation => operation.Split(':')[0] is "push" or "pop" or "mv")).ToArray();
        Assert.Equal(132, rows.Length);
        Assert.Equal(12, rows.Count(row => row[1].Contains("|cb:", StringComparison.Ordinal)));
        // 기대값을 다시 계산하지 않고 각 원문 입력을 실제 제어/효과음 본체에 전달한다.
        foreach (string[] row in rows) Replay(row, kind);
    }

    /// <summary>원문 명령열의 장치 사건·최종 다섯 전역·전체 소리 표를 대조한다.</summary>
    private static void Replay(string[] row, OriginalEdition edition)
    {
        Assert.Equal(13, row.Length);
        var scene = new Scene(edition);
        var tokens = new List<string>();
        // fixture의 전역 대입과 실제 호출을 원문 순서 그대로 소비한다.
        foreach (string operation in row[1].Split('|'))
        {
            string[] op = operation.Split(':');
            switch (op[0])
            {
                case "seed": scene.Seed(); break;
                case "g":
                    switch (op[1])
                    {
                        case "depth": scene.State.VolumeHoldDepth = Whole(op[2]); break;
                        case "musicinit": scene.State.MusicInitialized = Whole(op[2]) != 0; break;
                        case "initialized": scene.State.Initialized = Whole(op[2]) != 0; break;
                        default: throw new InvalidDataException("음소거 관측의 미지원 전역");
                    }
                    break;
                case "hr": Assert.Equal("gain", op[1]); scene.GainResult = Whole(op[2]); break;
                case "cb": scene.Callback = (op[1] == "music", Whole(op[2]), Whole(op[3])); break;
                case "master": scene.Effects.SetMasterVolume(Whole(op[1])); break;
                case "mv": scene.Control.SetVolume(Whole(op[1])); break;
                case "push": scene.Control.PushMute(); break;
                case "pop": scene.Control.PopMute(); break;
                default: throw new InvalidDataException("음소거 관측의 미지원 명령");
            }
            tokens.Add(scene.Take() + "=-");
        }
        Assert.Equal(row[2], string.Join('|', tokens));
        Assert.Equal(uint.Parse(row[5], CultureInfo.InvariantCulture), X86Fixture.Adler32(scene.List.Raw().Span));
        Assert.Equal(row[8..], new[] { scene.State.MasterVolume, scene.State.PendingMasterVolume, scene.State.VolumeHoldDepth,
            scene.State.MusicVolume, scene.State.PendingMusicVolume }.Select(value => value.ToString(CultureInfo.InvariantCulture)).ToArray());
    }

    /// <summary>준비 전/정지중 자연 관문은 채널을 요구하지 않고 선택된 실제 갱신만 현재값 저장 뒤 실패한다.</summary>
    [Fact]
    public void MissingSelectedChannelPreservesMusicStoreAndReleasesCommandLock()
    {
        var scene = new Scene(OriginalEdition.Patch1078, false);
        scene.State.MusicInitialized = false;
        scene.Control.SetVolume(17);
        scene.State.MusicInitialized = true;
        scene.Control.Stopping = true;
        scene.Control.SetVolume(18);
        scene.Control.Stopping = false;
        Assert.Throws<InvalidOperationException>(() => scene.Control.SetVolume(19));
        Assert.Equal(19, scene.State.MusicVolume);
        scene.State.MusicInitialized = false;
        scene.Control.SetVolume(20);
        Assert.Equal(20, scene.State.MusicVolume);
        Assert.Equal("-", scene.Take());
    }

    /// <summary>다른 효과음 전역을 가진 재생기와 음소거 제어의 결합을 실행 전 거부한다.</summary>
    [Fact]
    public void RejectsDifferentActualSoundOwner()
    {
        var scene = new Scene(OriginalEdition.Patch1078);
        Assert.Throws<ArgumentException>(() => new SoundMusicControl(OriginalEdition.Patch1078, new SoundState(), scene.Effects));
    }
}

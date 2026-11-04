using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

/// <summary>
/// 최초 Notify 1596관측을 제안 패치가 적용된 실제 목록 상태와 비교한다
/// (cpppj <c>tests/FortNotifyTests.cpp</c>를 직역했다).
/// </summary>
/// <remarks>2026-11-04 F2c-11에서 추가했다. 제품(<c>RawPlayerTemplateState</c>)은 0.128에서 옮겼고 이 단위에서 검사를 이었다. 원본 x86 fixture로 진입점 전체를 관통하므로 PORT_MAP 상태는 L1이다.</remarks>
public sealed class FortNotifyTests
{
    /// <summary>최초 격리 입력의 실제 관리자 포인터 비트다.</summary>
    private const uint NotifyManager11 = 0x12000100u;

    /// <summary>두 번째 격리 입력의 실제 관리자 포인터 비트다.</summary>
    private const uint NotifyManager22 = 0x12000200u;

    /// <summary>원본행별 장치의 지정 중지에 해당하는 중단 표식이다.</summary>
    private sealed class NotifyStop : Exception
    {
    }

    /// <summary>마지막 빈 칸도 유지하는 기록 필드 분리다.</summary>
    /// <param name="text">자를 문자열이다.</param>
    /// <param name="separator">구분자다.</param>
    /// <returns> 나눈 칸 목록이다.</returns>
    private static List<string> NotifyFields(string text, char separator)
    {
        List<string> result = [];
        int start = 0;
        // 원래 구분자의 순서를 보존한다
        for (; ; )
        {
            int end = text.IndexOf(separator, start);
            if (end < 0)
            {
                result.Add(text.Substring(start));
                return result;
            }
            result.Add(text.Substring(start, end - start));
            start = end + 1;
        }
    }

    /// <summary>원본 네 바이트만 리틀 엔디언 DWORD로 읽는다.</summary>
    /// <param name="bytes">읽을 버퍼다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>결합한 값이다.</returns>
    private static uint NotifyGet(byte[] bytes, int at)
    {
        uint value = 0;
        // 주변 기록은 읽지 않는다
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)bytes[at + index] << (index * 8);
        }
        return value;
    }

    /// <summary>지정 DWORD만 쓰고 이웃 필드는 보존한다.</summary>
    /// <param name="bytes">쓸 버퍼다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void NotifyPut(byte[] bytes, int at, uint value)
    {
        // 원본 네 바이트 폭을 유지한다
        for (int index = 0; index < 4; index++)
        {
            bytes[at + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>전체 물리 바이트를 원본 관측의 소문자 16진수로 표현한다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <returns>16진수 문자열이다.</returns>
    private static string NotifyHex(byte[] bytes)
    {
        const string digits = "0123456789abcdef";
        System.Text.StringBuilder result = new(bytes.Length * 2);
        // 미사용 꼬리까지 비교한다
        foreach (byte value in bytes)
        {
            result.Append(digits[value >> 4]);
            result.Append(digits[value & 15]);
        }
        return result.ToString();
    }

    /// <summary>명시 관리자 번호를 실제 raw 주소로 변환한다.</summary>
    /// <param name="manager">관리자 번호다.</param>
    /// <returns>raw 주소 비트다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">Notify 관리자 번호 범위 오류다.</exception>
    private static uint NotifyPointer(uint manager)
    {
        if (manager == 0)
        {
            return 0;
        }
        if (manager == 11)
        {
            return NotifyManager11;
        }
        if (manager == 22)
        {
            return NotifyManager22;
        }
        throw new ArgumentOutOfRangeException(nameof(manager), manager, "Notify 관리자 번호 범위");
    }

    /// <summary>raw 관리자 주소를 명시 하위 장치 번호로 변환한다.</summary>
    /// <param name="pointer">raw 주소 비트다.</param>
    /// <returns>관리자 번호다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">Notify 관리자 포인터 범위 오류다.</exception>
    private static uint NotifyManager(uint pointer)
    {
        if (pointer == 0)
        {
            return 0;
        }
        if (pointer == NotifyManager11)
        {
            return 11;
        }
        if (pointer == NotifyManager22)
        {
            return 22;
        }
        throw new ArgumentOutOfRangeException(nameof(pointer), pointer, "Notify 관리자 포인터 범위");
    }

    /// <summary>최초 입력의 실제 머리·이름과 진단·저장 응답만 제공한다.</summary>
    private sealed class NotifyDevices
    {
        /// <summary>최초 입력의 각 DWORD다.</summary>
        public uint Manager;

        /// <summary>이름 선택 번호다.</summary>
        public uint Name;

        /// <summary>내 것 플래그 입력이다.</summary>
        public uint Mine;

        /// <summary>임시 플래그 입력이다.</summary>
        public uint Transient;

        /// <summary>저장 방지 입력이다.</summary>
        public uint Prevent;

        /// <summary>임무 입력이다.</summary>
        public uint MissionInput;

        /// <summary>기존 클래스가 직접 빌리는 같은 현재 임무 내 저장소다.</summary>
        public bool Mission;

        /// <summary>Notify는 읽거나 쓰지 않는 기존 장기 로딩 깊이다.</summary>
        public uint Depth;

        /// <summary>다른 8칸도 보존을 확인하는 전체 Template 표다.</summary>
        public readonly byte[] Headers = new byte[180];

        /// <summary>첫 바이트 변경 외 모든 관리자 바이트도 비교한다.</summary>
        public readonly byte[][] Managers = [new byte[64], new byte[64]];

        /// <summary>같은 두 관리자의 실제 C문자열 저장소다.</summary>
        public readonly string[] Names = new string[2];

        /// <summary>이 Notify 검사에서 선택되지 않는 기존 State API 입력이다.</summary>
        public readonly PlayerTemplateStateStream[] States = new PlayerTemplateStateStream[9];

        /// <summary>명시 장치 변경이다.</summary>
        public readonly string Mutation;

        /// <summary>실제 발생한 순서다.</summary>
        public string Trace = "";

        /// <summary>최초 기록 입력열만 읽고 관측 기대값은 사용하지 않는다.</summary>
        /// <param name="input">쉼표 7칸 입력이다.</param>
        public NotifyDevices(string input)
        {
            List<string> fields = NotifyFields(input, ',');
            if (fields.Count != 7)
            {
                throw new InvalidOperationException("Notify 입력 칸 수 오류");
            }
            Manager = uint.Parse(fields[0], System.Globalization.CultureInfo.InvariantCulture);
            Name = uint.Parse(fields[1], System.Globalization.CultureInfo.InvariantCulture);
            Mine = uint.Parse(fields[2], System.Globalization.CultureInfo.InvariantCulture);
            Transient = uint.Parse(fields[3], System.Globalization.CultureInfo.InvariantCulture);
            Prevent = uint.Parse(fields[4], System.Globalization.CultureInfo.InvariantCulture);
            MissionInput = uint.Parse(fields[5], System.Globalization.CultureInfo.InvariantCulture);
            Mission = MissionInput != 0;
            Mutation = fields[6];
            // 전체 표의 미정 바이트 입력이다
            for (int index = 0; index < Headers.Length; index++)
            {
                Headers[index] = (byte)(index * 13 + 7);
            }
            NotifyPut(Headers, 20, NotifyPointer(Manager));
            NotifyPut(Headers, 24, 0x12345678u);
            NotifyPut(Headers, 28, Mine);
            NotifyPut(Headers, 32, Transient);
            NotifyPut(Headers, 36, Prevent);
            string[] first = ["", ".map", "map.fort", "\0hidden"];
            Names[0] = first[(int)Name];
            Names[1] = "B.fort";
            // 실제 관리자 이름·NUL 뒤 공간도 처음 패턴을 유지한다
            for (int index = 0; index < 2; index++)
            {
                Array.Fill(Managers[index], (byte)0xa5);
                for (int slot = 0; slot <= Names[index].Length; slot++)
                {
                    Managers[index][12 + slot] = slot == Names[index].Length ? (byte)0 : (byte)Names[index][slot];
                }
            }
            // 기본 스트림을 채운다
            for (int index = 0; index < States.Length; index++)
            {
                States[index] = new PlayerTemplateStateStream();
            }
        }

        /// <summary>한 시간순 기록에 파일명·진단·저장 진입을 기록한다.</summary>
        /// <param name="text">기록할 항목이다.</param>
        public void Record(string text)
        {
            if (Trace.Length != 0)
            {
                Trace += ';';
            }
            Trace += text;
        }

        /// <summary>지정 장치의 현재 필드 변경만 수행하고 이후 분기·재조회는 제품에 맡긴다.</summary>
        /// <param name="boundary">고를 경계 이름이다.</param>
        /// <exception cref="NotifyStop">원본행이 지정한 중단 지점이다.</exception>
        public void Mutate(string boundary)
        {
            // 여러 진단의 순차 변경도 독립 입력으로 처리한다
            foreach (string part in NotifyFields(Mutation, '+'))
            {
                int split = part.IndexOf('-');
                if (split < 0 || part.Substring(0, split) != boundary)
                {
                    continue;
                }
                string action = part.Substring(split + 1);
                if (action.StartsWith("manager", StringComparison.Ordinal))
                {
                    NotifyPut(Headers, 20, NotifyPointer(uint.Parse(action.Substring(7), System.Globalization.CultureInfo.InvariantCulture)));
                }
                else if (action.StartsWith("mine", StringComparison.Ordinal))
                {
                    NotifyPut(Headers, 28, uint.Parse(action.Substring(4), System.Globalization.CultureInfo.InvariantCulture));
                }
                else if (action.StartsWith("transient", StringComparison.Ordinal))
                {
                    NotifyPut(Headers, 32, uint.Parse(action.Substring(9), System.Globalization.CultureInfo.InvariantCulture));
                }
                else if (action.StartsWith("prevent", StringComparison.Ordinal))
                {
                    NotifyPut(Headers, 36, uint.Parse(action.Substring(7), System.Globalization.CultureInfo.InvariantCulture));
                }
                else if (action.StartsWith("mission", StringComparison.Ordinal))
                {
                    MissionInput = uint.Parse(action.Substring(7), System.Globalization.CultureInfo.InvariantCulture);
                    Mission = MissionInput != 0;
                }
                else if (action == "nameEmpty")
                {
                    Managers[0][12] = 0;
                    if (Names[0].Length != 0)
                    {
                        Names[0] = "\0" + Names[0].Substring(1);
                    }
                }
                else if (action == "nameDot")
                {
                    Managers[0][12] = (byte)'.';
                    if (Names[0].Length == 0)
                    {
                        Names[0] = ".";
                    }
                    else
                    {
                        Names[0] = "." + Names[0].Substring(1);
                    }
                }
                else if (action == "stop")
                {
                    throw new NotifyStop();
                }
            }
        }

        /// <summary>실제 현재 관리자의 첫 파일명 바이트 조회를 같은 관리자별 문자열로 연결한다.</summary>
        /// <param name="slot">조회할 칸이다.</param>
        /// <returns>관리자 이름이다.</returns>
        public string DeviceName(uint slot)
        {
            Assert.Equal(1u, slot);
            uint current = NotifyManager(NotifyGet(Headers, 20));
            if (current == 0)
            {
                throw new InvalidOperationException("Notify 패치가 첫 관리자 0을 조회 전에 거부하지 않았습니다");
            }
            uint index = current == 11 ? 0u : 1u;
            Record("name:" + current + ':' + Managers[index][12]);
            return Names[index];
        }

        /// <summary>원본행별 장치를 실제 클래스 진단 enum에 연결한다.</summary>
        /// <param name="reason">제품이 보고한 진단이다.</param>
        public void AssertDevice(PlayerTemplateStateAssertion reason)
        {
            uint line;
            string boundary;
            switch (reason)
            {
                case PlayerTemplateStateAssertion.DotFileName: line = 449; boundary = "dot"; break;
                case PlayerTemplateStateAssertion.MineTransientFlag: line = 451; boundary = "transient"; break;
                case PlayerTemplateStateAssertion.InMissionSave: line = 454; boundary = "mission"; break;
                case PlayerTemplateStateAssertion.MissingNotifyTemplate: line = 455; boundary = "missing"; break;
                default: throw new InvalidOperationException("Notify 검사 밖 State 진단");
            }
            Record("assert:" + line);
            Mutate(boundary);
        }

        /// <summary>제안 패치가 적용된 실제 클래스를 실행하고 첫 물리 경계와 지정 중단만 정규화한다.</summary>
        /// <returns>종료 상태 문자열이다.</returns>
        public string Run()
        {
            RawPlayerTemplateState state = new(
                OriginalEdition.Patch1078,
                new Memory<byte>(Headers),
                slot => States[(int)slot],
                slot => DeviceName(slot),
                () => Depth,
                value => Depth = value,
                () => Mission,
                reason => AssertDevice(reason),
                value => throw new InvalidOperationException("Notify는 로딩 깊이를 쓰지 않습니다"));
            state.BindFileSave(slot =>
            {
                Assert.Equal(1u, slot);
                Record("save:" + NotifyManager(NotifyGet(Headers, 20)));
                Mutate("save");
            });
            try
            {
                state.Changed(1);
                return "return";
            }
            catch (NotifyStop)
            {
                return "stop";
            }
            catch (ArgumentOutOfRangeException error)
            {
                if (error.Message.Contains("Player Notify manager0 파일명 첫BYTE 물리접근경계", StringComparison.Ordinal) != true)
                {
                    throw;
                }
                Record("fault:12:1");
                return "fault";
            }
        }

        /// <summary>전체 머리·관리자를 최초 관측과 같은 원래 순서로 표현한다.</summary>
        /// <returns>연결한 16진수다.</returns>
        public string Raw()
        {
            return NotifyHex(Headers) + NotifyHex(Managers[0]) + NotifyHex(Managers[1]);
        }
    }

    /// <summary>최초 798입력의 두 정밀도·정상·중단·물리 경계와 모든 장치 효과를 대조한다.</summary>
    [Fact]
    public void IndependentNative1596()
    {
        string[][] rows = Netstorm.Tests.X86Fixture.Read("fort-notify");
        HashSet<string> unique = [];
        int compared = 0;
        int faults = 0;
        int stops = 0;
        // 제품 실행 뒤에만 최초 관측열을 기대값으로 사용한다
        foreach (string[] row in rows)
        {
            Assert.Equal(6, row.Length);
            Assert.True(unique.Add(row[0] + "\t" + row[1]));
            NotifyDevices devices = new(row[1]);
            string status = devices.Run();
            Assert.Equal(row[2], status);
            Assert.Equal(row[3], devices.MissionInput.ToString(System.Globalization.CultureInfo.InvariantCulture));
            Assert.Equal(row[4], devices.Raw());
            Assert.Equal(row[5], devices.Trace);
            compared++;
            if (status == "fault")
            {
                faults++;
            }
            if (status == "stop")
            {
                stops++;
            }
        }
        Assert.Equal(1596, compared);
        Assert.Equal(128, faults);
        Assert.Equal(80, stops);
    }

    /// <summary>빈 정상 C문자열은 관문 뒤 저장 장치 누락 실패를 유지하고 선택 안 된 저장은 요구하지 않는다.</summary>
    [Fact]
    public void EmptyCStringKeepsSelectedSaveFailure()
    {
        byte[] headers = new byte[180];
        NotifyPut(headers, 20, 11);
        PlayerTemplateStateStream[] streams = new PlayerTemplateStateStream[9];
        // 기본 스트림을 채운다
        for (int index = 0; index < streams.Length; index++)
        {
            streams[index] = new PlayerTemplateStateStream();
        }
        string[] names = new string[9];
        Array.Fill(names, "");
        uint depth = 0;
        bool mission = false;
        RawPlayerTemplateState state = new(
            OriginalEdition.Patch1078,
            new Memory<byte>(headers),
            slot => streams[(int)slot],
            slot => names[(int)slot],
            () => depth,
            value => depth = value,
            () => mission,
            reason => throw new InvalidOperationException("빈 이름 자체는 진단 대상이 아닙니다"),
            value => throw new InvalidOperationException("Notify 로딩 깊이 변경 금지"));
        bool missing = false;
        try
        {
            state.Changed(1);
        }
        catch (InvalidOperationException error)
        {
            missing = error.Message.Contains("Player Template실제DataManager파일저장44d8d0/CD44cc40미연결", StringComparison.Ordinal);
        }
        Assert.True(missing);
        NotifyPut(headers, 28, 1);
        state.Changed(1);
        NotifyPut(headers, 28, 0);
        NotifyPut(headers, 36, 1);
        state.Changed(1);
    }
}

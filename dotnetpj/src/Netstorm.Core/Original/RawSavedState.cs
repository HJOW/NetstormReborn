// 일반 전투의 !state.dat 쓰기와 저장 상태 파일/8Player 복원을 현재 전역·파일 장치에 연결한다.
namespace Netstorm.Core.Original;

// 저장 상태 모듈을 담는 네임스페이스다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.

// 파일 서명이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
public static class SavedStateFormat
{
    /// <summary>!state.dat 서명 DWORD다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public const uint Signature = 0xa004e;

    /// <summary>판독 scratch 크기(392바이트)다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public const int ScratchBytes = 0x188;

    /// <summary>현재 Player 한 기록 크기다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public const int PlayerStride = 180;

    /// <summary>현재 Player 수다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public const int PlayerCount = 9;
}

// 같은 Client/Session의 현재 값을 선택 위치마다 다시 조회하는 쓰기 전역이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
public sealed class SavedStateWriteGlobals
{
    /// <summary>원문 바깥 관문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Func<bool>? MissionPost { get; set; }

    /// <summary>원문 PlayerKnown 관문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Func<bool>? PlayerKnown { get; set; }

    /// <summary>두 checking 위치다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Func<bool>? Checking { get; set; }

    /// <summary>현재 실제 localOwner 값이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Func<uint>? LocalOwner { get; set; }

    /// <summary>현재 실제 mySubscriber 값이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Func<uint>? MySubscriber { get; set; }

    /// <summary>현재 이름 NUL을 빌린다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Func<byte[]>? Handle { get; set; }

    /// <summary>현재 9×180바이트를 빌린다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Func<byte[]>? Players { get; set; }

    /// <summary>실제 restoreOwner 출력 쓰기다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Action<uint>? SetRestoreOwner { get; set; }

    /// <summary>실제 restoreSubscriber 출력 쓰기다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Action<uint>? SetRestoreSubscriber { get; set; }

    /// <summary>현재 NUL 포함 이름 복사 대상이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public byte[]? PreviousHandle { get; set; }

    /// <summary>각 write 직전에 읽는 세 전역이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Func<uint>?[] Tail { get; set; } = new Func<uint>?[3];

    /// <summary>마지막 DWORD는 현재 값이 0이 아닌지로만 쓴다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Func<double>? LoginEnd { get; set; }
}

// 파일 장치와 실제 SP 진단/시간/현재 Fort 이름의 쓰기 경계다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
public sealed class SavedStateWriteHooks
{
    /// <summary>같은 256바이트 물리이며 checking/복사 시 재조회한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Func<byte[]>? CurrentFort { get; set; }

    /// <summary>원문의 SP 추적 비교/잔여횟수/배열 진단 접두를 현재 실제 저장소에 실행한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Action? Diagnose { get; set; }

    /// <summary>실제 이름을 현재 위치에서 작성하고 연다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Action<uint, uint>? Open { get; set; }

    /// <summary>같은 File에 현재 물리/offset-1을 동기 기록한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Action<byte[], int>? Write { get; set; }

    /// <summary>정상/선택 예외에서 같은 File을 한 번 닫는다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Action? Close { get; set; }

    /// <summary>현재 벽시각이며 프레임 게임시각이 아니다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Func<double>? WallSeconds { get; set; }

    /// <summary>현재 PlayerKnown/owner를 다시 읽으며 미known은 0.0이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Func<float>? LocalSp { get; set; }

    /// <summary>State.cpp의 진단 뒤 계속 실행한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Action<string, uint>? Assertion { get; set; }
}

// 현재 Client/Session의 실제 출력 전역을 선택 위치에서 빌리는 판독 전역이다. 이 모듈은 같은 이름의 새 전역을 소유하지 않는다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
public sealed class SavedStateReadGlobals
{
    /// <summary>복귀 뒤 ffffffff를 먼저 쓰고 실제 4바이트 read를 받는다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Func<UIntBox>? Owner { get; set; }

    /// <summary>현재 복구 owner와 같은 레코드에서만 파일 ID를 받는다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Func<UIntBox>? Subscriber { get; set; }

    /// <summary>같은 NUL 출력 물리다. 새 Player 이름을 쓰기 전에 기존 이름을 복사한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Func<byte[]>? PreviousHandle { get; set; }

    /// <summary>현재 Player 배열을 매 레코드 7read 뒤 조회한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Func<byte[]>? Players { get; set; }

    /// <summary>직접 read 출력인 세 전역이다. 읽기 짧음은 현재 상위 바이트를 유지한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Func<UIntBox>?[] Tail { get; set; } = new Func<UIntBox>?[3];
}

// 실제 파일 객체/이름/Core 초기화/9×16 메타데이터의 판독 하위 장치를 선택 시 필수로 빌린다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
public sealed class SavedStateReadHooks
{
    /// <summary>NUL 포함 같은 살아 있는 경로 할당이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Func<byte[]>? Path { get; set; }

    /// <summary>파일 존재 관문이다. 디렉터리도 비영이며 0이면 다른 장치를 요구하지 않는다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Func<string, int>? Exists { get; set; }

    /// <summary>같은 24바이트 파일 object와 반환 토큰의 수명은 close까지 유지한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Func<string, byte[], uint, uint, uint>? Open { get; set; }

    /// <summary>대상 물리/요청 폭이다. 버퍼의 지정 구간에 직접 쓰며 부분 복사만 하고 나머지를 초기화하지 않는다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Func<uint, byte[], int, int, uint, uint>? Read { get; set; }

    /// <summary>같은 파일 object 해제다. 정상 꼬리 또는 실제 해당 선택 중단에서 한 번 호출한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Action<uint, byte[]>? Close { get; set; }

    /// <summary>실제 기존 pending 이름 helper다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Action<string>? Pending { get; set; }

    /// <summary>같은 지역 Core 10바이트 초기화다. 짧은 파일 read보다 먼저 매 레코드 실행한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Action<byte[]>? InitializeCore { get; set; }

    /// <summary>owner/ID/값으로 복제한 Core 10바이트다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    public Action<uint, uint, byte[]>? Metadata { get; set; }
}

// 일반 전투의 !state.dat 쓰기와 저장 상태 복원을 같은 파일 배치·순서로 실행한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
public static class RawSavedState
{
    /// <summary>
    /// 현재 전투에서만 새 wall을 읽고 만료하면 저장 뒤 또 읽어 10초를 예약한다.
    /// </summary>
    /// <param name="battle">현재 전투인가다.</param>
    /// <param name="deadline">다음 예약 시각이다.</param>
    /// <param name="wallSeconds">현재 벽시각 읽기다.</param>
    /// <param name="save">저장 실행이다.</param>
    public static void PollSavedStateWrite(bool battle, ref double deadline, Func<double>? wallSeconds, Action? save)
    {
        if (!battle)
        {
            return;
        }

        double now = Need(wallSeconds, "저장상태 writer 실제 장치 미연결")();
        if (deadline <= now)
        {
            Need(save, "저장상태 writer 실제 장치 미연결")();
            deadline = Need(wallSeconds, "저장상태 writer 실제 장치 미연결")() + 10.0;
        }
    }

    /// <summary>
    /// post 또는 미known이면 장치 없이 반환하며 부분 쓰기를 전체 트랜잭션으로 되돌리지 않는다.
    /// </summary>
    /// <param name="globals">현재 값 전역이다.</param>
    /// <param name="hooks">파일 장치 경계다.</param>
    public static void WriteSavedState(SavedStateWriteGlobals globals, SavedStateWriteHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        if (Need(globals.MissionPost, "저장상태 writer 실제 장치 미연결")() ||
            !Need(globals.PlayerKnown, "저장상태 writer 실제 장치 미연결")())
        {
            return;
        }

        Need(hooks.Diagnose, "저장상태 writer 실제 장치 미연결")();
        Need(hooks.Open, "저장상태 writer 실제 장치 미연결")(2u, 1u);
        bool active = true;
        // 원문 SEH의 -1 설정처럼 예외 재진입의 중복 닫기를 막는 닫기다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
        void Close()
        {
            active = false;
            Need(hooks.Close, "저장상태 writer 실제 장치 미연결")();
        }

        // 파일 쓰기 반환으로 새 성공/실패 분기를 만들지 않는 쓰기다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
        void WriteBytes(ReadOnlySpan<byte> bytes) => Need(hooks.Write, "저장상태 writer 실제 장치 미연결")(bytes.ToArray(), -1);

        // 원문의 개별 DWORD 지역 값을 동기 기록하는 쓰기다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
        void Dword(uint value)
        {
            byte[] bytes = new byte[4];
            // 네 바이트 뒤의 다음 필드/현재 전역은 아직 조회하지 않는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
            for (int index = 0; index < 4; ++index)
            {
                bytes[index] = (byte)(value >> (index * 8));
            }

            WriteBytes(bytes);
        }

        try
        {
            if (Need(globals.Checking, "저장상태 writer 실제 장치 미연결")())
            {
                // checking은 write할 이름의 나중 복사와 별도 현재 조회다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
                byte[] name = Need(hooks.CurrentFort, "저장상태 writer 실제 장치 미연결")();
                // 점의 첫 발견에서 진단하고 계속하는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
                foreach (byte current in name)
                {
                    if (current == 0)
                    {
                        break;
                    }

                    if (current == (byte)'.')
                    {
                        Need(hooks.Assertion, "저장상태 writer 실제 장치 미연결")("!(strchr(curFort,'.'))", 0xf79u);
                        break;
                    }
                }
            }

            Dword(SavedStateFormat.Signature);
            byte[] fort = new byte[256];
            CopyName(fort, Need(hooks.CurrentFort, "저장상태 writer 실제 장치 미연결")());
            WriteBytes(fort);
            uint owner = Need(globals.LocalOwner, "저장상태 writer 실제 장치 미연결")();
            Need(globals.SetRestoreOwner, "저장상태 writer 실제 장치 미연결")(owner);
            Dword(owner);
            Need(globals.SetRestoreSubscriber, "저장상태 writer 실제 장치 미연결")(Need(globals.MySubscriber, "저장상태 writer 실제 장치 미연결")());
            CopyName(Need(globals.PreviousHandle, "저장상태 writer 실제 장치 미연결"), Need(globals.Handle, "저장상태 writer 실제 장치 미연결")());
            Dword(Truncate(Need(hooks.WallSeconds, "저장상태 writer 실제 장치 미연결")()));
            Dword(Truncate(Need(hooks.LocalSp, "저장상태 writer 실제 장치 미연결")()));
            uint serverCount = 0u;
            // 현재 Player base를 매 반복 읽어 그 기록의 실제 일곱 필드와 뒤 flags 8을 순서대로 쓰는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
            for (uint player = 1u; player < 9u; ++player)
            {
                byte[] records = Need(globals.Players, "저장상태 writer 실제 장치 미연결")();
                if (records.Length != SavedStateFormat.PlayerCount * SavedStateFormat.PlayerStride)
                {
                    throw new ArgumentOutOfRangeException(nameof(globals), "저장상태 writer 실제 9 Player 누락");
                }

                ReadOnlySpan<byte> row = records.AsSpan(unchecked((int)(player * (uint)SavedStateFormat.PlayerStride)), SavedStateFormat.PlayerStride);
                Dword(player);
                Dword(ReadDword(row, 0x2c));
                WriteBytes(row.Slice(12, 30));
                WriteBytes(row.Slice(0x30, 30));
                Dword(ReadDword(row, 0x50));
                Dword(ReadDword(row, 0x58));
                WriteBytes(row.Slice(0x6c, 10));
                if ((row[0x50] & 8u) != 0u)
                {
                    ++serverCount;
                }
            }

            if (Need(globals.Checking, "저장상태 writer 실제 장치 미연결")() && serverCount > 1u)
            {
                Need(hooks.Assertion, "저장상태 writer 실제 장치 미연결")("serverCount <= 1", 0xfa3u);
            }

            if (globals.Tail is null || globals.Tail.Length != 3)
            {
                throw new ArgumentException("저장상태 writer 꼬리 전역 3개 누락", nameof(globals));
            }

            // 같은 세 전역을 각 write 위치에서 현재 조회하는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
            foreach (Func<uint>? field in globals.Tail)
            {
                Dword(Need(field, "저장상태 writer 실제 장치 미연결")());
            }

            Dword(Need(globals.LoginEnd, "저장상태 writer 실제 장치 미연결")() != 0.0 ? 1u : 0u);
            Close();
        }
        catch
        {
            // 선택 예외에서 파일만 반납하며 Player/이름/진단의 부분 효과는 되돌리지 않는다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
            if (active)
            {
                Close();
            }

            throw;
        }
    }

    /// <summary>
    /// 없음/잘못된 서명/점이름은 0, 종료 DWORD 0이고 참가자 2명 이상인 경우만 signed 저장 시각이다.
    /// scratch는 호출 전 명시 초기화한 392바이트다. 파일 짧음의 잔여와 부분 Player/이름/전역 효과를 복구하지 않는다.
    /// owner가 실제 Player 물리 밖이거나 NUL 없는 문자열이 물리 끝을 넘으면 host 계약 오류로 거부한다.
    /// </summary>
    /// <param name="scratch">명시 초기 stack 392바이트다.</param>
    /// <param name="globals">출력 전역이다.</param>
    /// <param name="hooks">파일 하위 장치다.</param>
    /// <returns>참가자 2명 이상일 때의 signed 저장 시각, 그 밖은 0이다.</returns>
    public static int ReadSavedState(byte[] scratch, SavedStateReadGlobals globals, SavedStateReadHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(scratch);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(hooks);
        string path = ReadString(Need(hooks.Path, "저장상태 reader의 실제 선택 장치 누락")());
        if (Need(hooks.Exists, "저장상태 reader의 실제 선택 장치 누락")(path) == 0)
        {
            return 0;
        }

        if (scratch.Length != SavedStateFormat.ScratchBytes)
        {
            throw new ArgumentException("저장상태 reader scratch 392바이트 누락", nameof(scratch));
        }

        if (globals.Tail is null || globals.Tail.Length != 3)
        {
            throw new ArgumentException("저장상태 reader 꼬리 전역 3개 누락", nameof(globals));
        }

        // 원문은 파일 생성자 앞에 두 마지막 DWORD만 0으로 쓴다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
        PutDword(scratch, 0x1c, 0u);
        PutDword(scratch, 0x20, 0u);
        // 같은 24바이트 파일 object를 open부터 close까지 같은 저장소로 빌리는 버퍼다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
        byte[] fileObject = new byte[24];
        Array.Copy(scratch, 0x0c, fileObject, 0, 24);
        // 생성자 정상 복귀 뒤에만 파일 해제의 책임이 생긴다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
        uint file = Need(hooks.Open, "저장상태 reader의 실제 선택 장치 누락")(path, fileObject, 0u, 0u);
        bool active = true;
        // 해제 전에 원문 state를 -1로 바꾸는 것처럼 중복 해제를 막는 닫기다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
        void Close()
        {
            active = false;
            Need(hooks.Close, "저장상태 reader의 실제 선택 장치 누락")(file, fileObject);
        }

        // 요청 폭은 실제 구간 길이이며 부족 반환을 0패딩/실패 변환하지 않는다. 훅이 실제 저장소에 직접 쓰므로 부모는 훅 반환 뒤 대상을 다시 만지지 않는다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
        void ReadAt(byte[] array, int offset, int length)
        {
            At(array, offset, length);
            Need(hooks.Read, "저장상태 reader의 실제 선택 장치 누락")(file, array, offset, length, 0xffffffffu);
        }

        // 실제 고정 DWORD의 바이트 표현을 사본 없이 직접 read에 빌리는 읽기다. 중단 시에도 쓴 바이트는 DWORD에 남긴다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
        void ReadBox(UIntBox box)
        {
            byte[] buffer = BitConverter.GetBytes(box.Value);
            try
            {
                uint count = Need(hooks.Read, "저장상태 reader의 실제 선택 장치 누락")(file, buffer, 0, 4, 0xffffffffu);
                int copy = (int)Math.Min(count, 4u);
                byte[] current = BitConverter.GetBytes(box.Value);
                Array.Copy(buffer, 0, current, 0, copy);
                box.Value = BitConverter.ToUInt32(current, 0);
            }
            catch
            {
                box.Value = BitConverter.ToUInt32(buffer, 0);
                throw;
            }
        }

        try
        {
            ReadAt(scratch, 0x40, 4);
            if (GetDword(scratch, 0x40) != SavedStateFormat.Signature)
            {
                Close();
                return 0;
            }

            ReadAt(scratch, 0x88, 256);
            if (FindDot(scratch.AsSpan(0x88)))
            {
                Close();
                return 0;
            }

            Need(hooks.Pending, "저장상태 reader의 실제 선택 장치 누락")(ReadString(scratch.AsSpan(0x88)));
            UIntBox restoreOwner = Need(globals.Owner, "저장상태 reader의 실제 선택 장치 누락")();
            restoreOwner.Value = 0xffffffffu;
            ReadBox(restoreOwner);
            ReadAt(scratch, 0x3c, 4);
            ReadAt(scratch, 0x44, 4);
            uint participants = 0u;
            // 원문은 길이나 내용에 따른 조기 레코드 종료 없이 8회 반복한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
            PutDword(scratch, 0x30, 8u);
            do
            {
                // 짧은 파일 read보다 먼저 매 레코드 실행하는 초기화다. 훅에는 임시 버퍼를 주고 성공·예외 모두 돌려받아 실제 저장소에 되돌린다(span 대리자 불가 대응). 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
                byte[] coreWindow = scratch.AsSpan(0x24, 10).ToArray();
                try
                {
                    Need(hooks.InitializeCore, "저장상태 reader의 실제 선택 장치 누락")(coreWindow);
                }
                finally
                {
                    Array.Copy(coreWindow, 0, scratch, 0x24, 10);
                }
                ReadAt(scratch, 4, 4);
                ReadAt(scratch, 8, 4);
                ReadAt(scratch, 0x48, 30);
                ReadAt(scratch, 0x68, 30);
                ReadAt(scratch, 0, 4);
                ReadAt(scratch, 0x38, 4);
                ReadAt(scratch, 0x24, 10);
                uint number = GetDword(scratch, 4);
                uint flags = GetDword(scratch, 0);
                // 매 반복 같은 지역 초기화/일곱 read 뒤 현재 Player base를 조회한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
                byte[] players = Need(globals.Players, "저장상태 reader의 실제 선택 장치 누락")();
                int offset = unchecked((int)(number * (uint)SavedStateFormat.PlayerStride));
                uint identity = GetDword(scratch, 8);
                PutDword(scratch, 0, flags & 0xffffffbfu);
                PutDword(players, offset + 0x2c, identity);
                if (GetDword(scratch, 4) == Need(globals.Owner, "저장상태 reader의 실제 선택 장치 누락")().Value)
                {
                    // 새 이름 전 같은 이전 Player 이름을 보존하는 복사다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
                    Need(globals.Subscriber, "저장상태 reader의 실제 선택 장치 누락")().Value = GetDword(scratch, 8);
                    CopyRange(Need(globals.PreviousHandle, "저장상태 reader의 실제 선택 장치 누락")(), 0, players, offset + 0x0c);
                }

                CopyRange(players, offset + 0x0c, scratch, 0x48);
                CopyRange(players, offset + 0x30, scratch, 0x68);
                PutDword(players, offset + 0x50, GetDword(scratch, 0));
                PutDword(players, offset + 0x58, GetDword(scratch, 0x38));
                PutDword(players, offset + 0x6c, GetDword(scratch, 0x24));
                PutDword(players, offset + 0x70, GetDword(scratch, 0x28));
                // 원문 Core는 4/4/2로 쓰며 2바이트 padding은 포함하지 않는 복사다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
                At(players, offset + 0x74, 2)[0] = scratch[0x2c];
                At(players, offset + 0x74, 2)[1] = scratch[0x2d];
                // 원문 인자 구조체의 같은 10바이트 값 복제다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
                byte[] core = scratch.AsSpan(0x24, 10).ToArray();
                Need(hooks.Metadata, "저장상태 reader의 실제 선택 장치 누락")(GetDword(scratch, 4), GetDword(scratch, 8), core);
                // 메타데이터 callback 뒤 지역 flags를 다시 읽는 계수다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
                uint currentFlags = GetDword(scratch, 0);
                if ((currentFlags & 1u) != 0u && (currentFlags & 0x200u) == 0u)
                {
                    ++participants;
                }

                PutDword(scratch, 0x30, GetDword(scratch, 0x30) - 1u);
            }
            while (GetDword(scratch, 0x30) != 0u);

            // 같은 세 전역의 부족 읽기 상위 바이트도 보존하는 꼬리 읽기다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
            foreach (Func<UIntBox>? current in globals.Tail)
            {
                ReadBox(Need(current, "저장상태 reader의 실제 선택 장치 누락")());
            }

            PutDword(scratch, 0x34, 0u);
            ReadAt(scratch, 0x34, 4);
            if (GetDword(scratch, 0x34) != 0u)
            {
                Close();
                return 0;
            }

            uint result = participants >= 2u ? GetDword(scratch, 0x3c) : 0u;
            Close();
            // close가 저장 시각을 바꿔도 원문처럼 미리 보관한 결과를 반환한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
            return unchecked((int)result);
        }
        catch
        {
            // 실제 해당 선택 중단에서 파일만 해제하고 원래 부분 중단을 전파한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
            if (active)
            {
                Close();
            }

            throw;
        }
    }

    /// <summary>실제 선택된 하위만 필수로 요구한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="callback">확인할 장치다.</param>
    /// <param name="message">누락 메시지다.</param>
    /// <returns>연결된 장치다.</returns>
    /// <exception cref="InvalidOperationException">선택된 실제 하위 미연결이다.</exception>
    private static T Need<T>(T? callback, string message)
        where T : class
    {
        if (callback is null)
        {
            throw new InvalidOperationException(message);
        }

        return callback;
    }

    /// <summary>현재 Player DWORD를 little endian으로 읽는다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>조합한 값이다.</returns>
    private static uint ReadDword(ReadOnlySpan<byte> bytes, int offset)
    {
        if (unchecked((uint)offset) + 4u > (uint)bytes.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "저장상태 writer DWORD 물리 범위");
        }

        uint value = 0u;
        // 원문의 정확 네 바이트만 읽는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
        for (int index = 0; index < 4; ++index)
        {
            value |= (uint)bytes[offset + index] << (index * 8);
        }

        return value;
    }

    /// <summary>실제 한 바이트 복사/NUL 순서와 실패 전 부분 출력을 보존한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    /// <param name="target">쓸 대상이다.</param>
    /// <param name="source">읽을 원본이다.</param>
    private static void CopyName(Span<byte> target, ReadOnlySpan<byte> source)
    {
        // NUL을 포함해 복사하며 물리 끝을 넘어 미정 메모리를 읽지 않는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
        for (int index = 0; ; ++index)
        {
            if (index >= source.Length || index >= target.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(target), "저장상태 writer 이름 NUL/물리 범위");
            }

            byte value = source[index];
            target[index] = value;
            if (value == 0)
            {
                return;
            }
        }
    }

    /// <summary>원본 _ftol의 signed64 정수부/하위 DWORD와 범위 밖 indefinite 하위 0이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    /// <param name="value">자를 값이다.</param>
    /// <returns>하위 DWORD다.</returns>
    private static uint Truncate(double value)
    {
        if (!double.IsFinite(value) || value >= 9223372036854775808.0 || value < -9223372036854775808.0)
        {
            return 0u;
        }

        return unchecked((uint)(long)value);
    }

    /// <summary>선택한 원문 접근 폭만 검사한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    /// <param name="bytes">고를 바이트다.</param>
    /// <param name="at">고를 위치다.</param>
    /// <param name="width">고를 폭이다.</param>
    /// <returns>고른 구간이다.</returns>
    private static Span<byte> At(byte[] bytes, int at, int width)
    {
        if (at < 0 || width < 0 || at > bytes.Length || width > bytes.Length - at)
        {
            throw new ArgumentOutOfRangeException(nameof(at), "저장상태 reader의 명시 물리 범위");
        }

        return bytes.AsSpan(at, width);
    }

    /// <summary>원문 little endian의 현재 지역 DWORD를 읽는다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    /// <returns>조합한 값이다.</returns>
    private static uint GetDword(byte[] bytes, int at)
    {
        ReadOnlySpan<byte> field = At(bytes, at, 4);
        uint result = 0u;
        // 이웃 필드의 초기 stack 값은 읽지 않는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
        for (int index = 0; index < 4; ++index)
        {
            result |= (uint)field[index] << (index * 8);
        }

        return result;
    }

    /// <summary>지역/Player의 실제 지정 DWORD만 쓴다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void PutDword(byte[] bytes, int at, uint value)
    {
        Span<byte> field = At(bytes, at, 4);
        // 부분 전역과 나머지 Player는 그대로 두는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
        for (int index = 0; index < 4; ++index)
        {
            field[index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>실제 NUL까지의 현재 문자열 뷰이며 물리 끝 밖 검색은 host 계약 오류다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <returns>NUL 앞 문자열이다.</returns>
    private static string ReadString(ReadOnlySpan<byte> bytes)
    {
        // 원문 char 포인터의 첫 NUL 위치만 따르는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
        for (int index = 0; index < bytes.Length; ++index)
        {
            if (bytes[index] == 0)
            {
                return System.Text.Encoding.Latin1.GetString(bytes.Slice(0, index));
            }
        }

        throw new ArgumentOutOfRangeException(nameof(bytes), "저장상태 reader의 NUL 없는 물리 문자열");
    }

    /// <summary>점 또는 NUL을 먼저 만난 위치에서 멈춘다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    /// <param name="bytes">고를 바이트다.</param>
    /// <returns>점을 먼저 만나면 참이다.</returns>
    private static bool FindDot(ReadOnlySpan<byte> bytes)
    {
        // 점 뒤 NUL이 없어도 즉시 실패 분기로 갈 수 있는 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
        foreach (byte current in bytes)
        {
            if (current == (byte)'.')
            {
                return true;
            }

            if (current == 0)
            {
                return false;
            }
        }

        throw new ArgumentOutOfRangeException(nameof(bytes), "저장상태 reader의 점 검색 물리 끝");
    }

    /// <summary>NUL 포함 복사는 한 바이트씩 실행하여 물리 실패 전까지의 부분 쓰기를 보존한다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.</summary>
    /// <param name="destination">쓸 대상이다.</param>
    /// <param name="output">쓸 위치다.</param>
    /// <param name="source">읽을 원본이다.</param>
    /// <param name="input">읽을 위치다.</param>
    private static void CopyRange(byte[] destination, int output, byte[] source, int input)
    {
        // 원문처럼 읽기→한 바이트 쓰기→NUL 판정 순서인 반복문이다. 2026-11-07 F2d 저장 나머지 이식에서 추가됐다.
        for (; ; )
        {
            if (input >= source.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(source), "저장상태 reader의 문자열 읽기 물리 끝");
            }

            byte value = source[input++];
            if (output >= destination.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(destination), "저장상태 reader의 문자열 쓰기 물리 끝");
            }

            destination[output++] = value;
            if (value == 0)
            {
                return;
            }
        }
    }
}

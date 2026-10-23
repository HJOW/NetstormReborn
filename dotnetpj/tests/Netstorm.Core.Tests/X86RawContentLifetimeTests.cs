using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 내용물 수명(RawContentLifetime)을 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/ContentLifetimeTests.cpp 의 ContentLifetime_IndependentPeFixture 와 같다:
/// 실제 풀에 부모(100)와 ContentForm(80)을 두고, 삭제·연결 검사·부착마다 커서 취소, 너겟 로컬/원격 알림,
/// 선지급 지갑, SP 가산, 위치 소리를 순서대로 기록한다. 콜백은 원본 관찰과 같은 변형만 슬롯에 적용한다.
/// 호출마다 반환 값·효과 문자열·부모 알림 문자열·관찰 슬롯 전체 바이트·풀 메타 11개를 비교한다.
/// 기준 판본은 10.78 이며 originalCD 행은 읽지 않는다 (W03 내용물 수명 조각, 2026-10-23).
/// </summary>
public sealed class X86RawContentLifetimeTests
{
    /// <summary>10.78 행의 판본 이름. 그 밖은 CD 10.72 다.</summary>
    private const string PatchEdition = "originals";

    /// <summary>CD 10.72 행의 판본 이름. 이 조각은 옮기지 않았으므로 세기만 한다.</summary>
    private const string CdEdition = "originalCD";

    /// <summary>한 판본의 행 수. 이 조각이 읽지 않는 판본도 같아야 자료가 바뀌지 않았다.</summary>
    private const int RowsPerEdition = 198;

    /// <summary>장면이 쓰는 풀의 슬롯 수(원본 관찰과 같다).</summary>
    private const int PoolCapacity = 32768;

    /// <summary>삭제·부착 대상 ContentForm 번호.</summary>
    private const int Form = 80;

    /// <summary>원본 관찰이 쓰는 삭제·부착 플래그.</summary>
    private const uint TestFlags = 0x10;

    /// <summary>fixture 의 열 수.</summary>
    private const int Columns = 14;

    /// <summary>슬롯의 16비트 값을 리틀 엔디언으로 읽는다.</summary>
    /// <param name="pool">읽을 풀.</param>
    /// <param name="sid">슬롯 번호.</param>
    /// <param name="offset">슬롯 안 시작 위치.</param>
    private static ushort Word(RawSidPool pool, int sid, int offset)
    {
        ReadOnlySpan<byte> raw = pool.Slot(sid);
        return (ushort)(raw[offset] | (raw[offset + 1] << 8));
    }

    /// <summary>16비트 값을 리틀 엔디언으로 쓴다(원본 관찰 변형 입력용).</summary>
    /// <param name="raw">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void PutWord(Span<byte> raw, int offset, uint value)
    {
        raw[offset] = (byte)value;
        raw[offset + 1] = (byte)(value >> 8);
    }

    /// <summary>32비트 값을 리틀 엔디언으로 쓴다(원본 관찰의 좌표 변형 입력용).</summary>
    /// <param name="raw">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void PutDword(Span<byte> raw, int offset, uint value)
    {
        // 네 바이트를 낮은 바이트부터 쓴다
        for (int index = 0; index < 4; index++)
        {
            raw[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>슬롯 전체를 소문자 hex 로 만든다.</summary>
    /// <param name="span">슬롯 바이트.</param>
    private static string Hex(ReadOnlySpan<byte> span) => Convert.ToHexStringLower(span.ToArray());

    /// <summary>한 행의 풀·모듈·콜백을 소유하는 장면.</summary>
    private sealed class Scene
    {
        /// <summary>ContentForm 슬롯과 부모 슬롯을 가진 풀.</summary>
        public readonly RawSidPool Pool;

        /// <summary>타입 번호 순서의 타입 레코드. 콜백이 값을 바꾸면 그대로 반영된다.</summary>
        public readonly OriginalTypeRecord[] Types;

        /// <summary>공통 삭제 본체. 깊이 카운터를 그대로 대조한다.</summary>
        public readonly RawSquidDestroy Destroy;

        /// <summary>실제 월드의 전투 전역.</summary>
        public readonly ContentFormState FormState = new();

        /// <summary>삭제 중 효과가 갱신하는 전역.</summary>
        public readonly ContentLifetimeState State;

        /// <summary>외부 효과 문자열(원본 관찰과 같은 순서).</summary>
        public readonly List<string> Effects = [];

        /// <summary>부모 알림 문자열.</summary>
        public readonly List<string> Notices = [];

        /// <summary>부착·연결 검사를 공유하는 ContentForm 처리기.</summary>
        public readonly RawContentFormHost Forms;

        /// <summary>검증 대상 내용물 수명.</summary>
        public readonly RawContentLifetime Lifetime;

        /// <summary>원본 관찰이 지정한 변형 번호.</summary>
        private readonly uint _mutation;

        /// <summary>삭제·부착의 부모 번호(+16 WORD).</summary>
        private int _parent;

        /// <summary>한 행의 입력으로 장면을 만든다.</summary>
        /// <param name="server">서버 권한 풀인지.</param>
        /// <param name="input">슬롯 입력 문자열(sid:hex;...).</param>
        /// <param name="costBits">너겟 타입(154) 비용의 float 비트.</param>
        /// <param name="owner">로컬 소유자 번호.</param>
        /// <param name="boss">다른 소유자에게 알릴 권한.</param>
        /// <param name="cursor">잡고 있는 내용물 번호(0 이면 없음).</param>
        /// <param name="mutation">원본 관찰 변형 번호.</param>
        public Scene(bool server, string input, uint costBits, uint owner, bool boss, int cursor, uint mutation)
        {
            _mutation = mutation;
            Pool = new RawSidPool(PoolCapacity, server);
            Types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
            // 타입 표는 값 객체 레코드로 채운다
            for (int index = 0; index < Types.Length; index++)
            {
                Types[index] = new OriginalTypeRecord();
            }
            Types[154].Cost = BitConverter.UInt32BitsToSingle(costBits);
            Types[90].FootX = 3;
            Types[90].FootY = 5;
            // 부착 장면의 전투·포획 조건은 관찰 입력이 정한다
            if (mutation >= 100)
            {
                FormState.InBattle = ((mutation - 100) & 4) != 0;
                Types[80].Flags2 = ((mutation - 100) & 2) != 0 ? 0x200000u : 0u;
                Types[90].Flags2 = ((mutation - 100) & 1) != 0 ? 0x10000u : 0u;
            }
            // 주어진 슬롯 바이트를 손실 없이 놓는다
            foreach (string item in input.Split(';'))
            {
                string[] field = item.Split(':');
                int sid = X86Fixture.Int(field[0]);
                byte[] bytes = X86Fixture.Bytes(field[1]);
                Span<byte> raw = Pool.RawSlot(sid);
                Assert.Equal(raw.Length, bytes.Length);
                bytes.CopyTo(raw);
            }
            State = new ContentLifetimeState
            {
                Cursor = cursor,
                NuggetType = 154,
                LocalOwner = owner,
                Notice = 0x1234,
                Boss = boss,
            };
            Destroy = new RawSquidDestroy(Pool, new RawUnpop(Pool, new RawSquidHash(), new byte[65536]), Types);
            Forms = new RawContentFormHost(Pool, Types, Notice, null, FormState, Captured);
            Lifetime = new RawContentLifetime(Pool, Types, Forms, Destroy, State, Hooks());
            _parent = Word(Pool, Form, 16);
        }

        /// <summary>외부 효과 문자열을 세미콜론으로 잇는다.</summary>
        public string EffectsText() => string.Join(';', Effects);

        /// <summary>부모 알림 문자열을 세미콜론으로 잇는다.</summary>
        public string NoticesText() => string.Join(';', Notices);

        /// <summary>지정 슬롯의 현재 전체 바이트를 hex 로 만든다.</summary>
        /// <param name="sid">슬롯 번호.</param>
        public string HexSlot(int sid) => Hex(Pool.Slot(sid));

        /// <summary>fixture 연산을 실행하고 결과 값을 돌려준다.</summary>
        /// <param name="op">Destroy·Linked·Attach 가운데 하나.</param>
        public uint Apply(string op)
        {
            switch (op)
            {
                case "Destroy":
                    Lifetime.Destroy(Form, TestFlags);
                    return 0;
                case "Linked":
                    return Forms.Linked(Form) ? 1u : 0u;
                case "Attach":
                    Forms.Attach(Form, _parent, TestFlags);
                    return 0;
                default:
                    throw new InvalidOperationException($"Content fixture 연산 오류: {op}");
            }
        }

        /// <summary>외부 효과 당시 전체 슬롯·전역·현재 타입 비용을 관찰한다.</summary>
        private string Snapshot()
        {
            uint cost = BitConverter.SingleToUInt32Bits(Types[(int)State.NuggetType].Cost);
            byte[] costBytes = new byte[4];
            // 원본은 비용 float 을 리틀 엔디언 4바이트로 남긴다
            for (int index = 0; index < 4; index++)
            {
                costBytes[index] = (byte)(cost >> (index * 8));
            }
            return $"{Hex(Pool.Slot(Form))}:{Hex(Pool.Slot(_parent))}:{State.LocalOwner}:{(State.Boss ? 1 : 0)}:{State.NuggetType}:{Hex(costBytes)}";
        }

        /// <summary>부모의 부착(1)·제거(2) 알림을 그 시점의 두 슬롯과 함께 기록한다.</summary>
        /// <param name="parent">알림을 받는 부모 번호.</param>
        /// <param name="form">부착·제거된 Form 번호.</param>
        /// <param name="reason">1 = 부착, 2 = 제거.</param>
        private void Notice(int parent, int form, uint reason)
        {
            Notices.Add($"content:{parent}:{form}:{reason}:{Hex(Pool.Slot(parent))}:{Hex(Pool.Slot(form))}");
        }

        /// <summary>전투 사제 포획 알림을 기록한다.</summary>
        /// <param name="parent">내용물을 담은 부모 번호.</param>
        /// <param name="previousOwner">내용물 +12 하위 7비트의 이전 소유자.</param>
        private void Captured(int parent, uint previousOwner)
        {
            Effects.Add($"capture:{parent}:{previousOwner}:{Snapshot()}");
        }

        /// <summary>원본 관찰과 같은 외부 경계를 만든다. 콜백은 관찰이 지정한 변형만 슬롯에 적용한다.</summary>
        private ContentLifetimeHooks Hooks()
        {
            var hooks = new ContentLifetimeHooks
            {
                CancelCursor = () =>
                {
                    Effects.Add($"cancel:{Snapshot()}");
                    State.Cursor = 0;
                    // 변형 1 은 취소 뒤 종류 WORD 를 너겟으로 바꾼다
                    if (_mutation == 1)
                    {
                        PutWord(Pool.RawSlot(Form), 18, 154);
                    }
                },
                LocalNotice = command =>
                {
                    Effects.Add($"local:{command}:{Snapshot()}");
                    switch (_mutation)
                    {
                        case 2:
                            Pool.RawSlot(Form)[22] = 1;
                            break;
                        case 3:
                            Pool.RawSlot(_parent)[34] = 3;
                            break;
                        case 7:
                            Pool.RawSlot(_parent)[11] = 2;
                            break;
                        case 8:
                            PutWord(Pool.RawSlot(Form), 18, 153);
                            break;
                        default:
                            break;
                    }
                },
                RemoteNotice = owner => Effects.Add($"remote:{owner}:{Snapshot()}"),
                Wallet = (owner, amount) =>
                {
                    Effects.Add($"wallet:{owner}:{unchecked((uint)amount)}:{Snapshot()}");
                    if (_mutation == 4)
                    {
                        Pool.RawSlot(_parent)[34] = 2;
                        Types[154].Cost = 125.75f;
                    }
                    else if (_mutation == 5)
                    {
                        State.LocalOwner = 2;
                    }
                },
                AddSp = (owner, amount, local) =>
                {
                    Effects.Add($"sp:{owner}:{BitConverter.SingleToUInt32Bits(amount)}:{(local ? 1 : 0)}:{Snapshot()}");
                    // 변형 6 은 SP 뒤 중심 계산이 읽는 부모 좌표를 바꾼다
                    if (_mutation == 6)
                    {
                        PutDword(Pool.RawSlot(_parent), 14, BitConverter.SingleToUInt32Bits(200.5f));
                        PutDword(Pool.RawSlot(_parent), 18, BitConverter.SingleToUInt32Bits(60.25f));
                    }
                },
                Sound = (x, y) => Effects.Add($"sound:{BitConverter.SingleToUInt32Bits(x)}:{BitConverter.SingleToUInt32Bits(y)}:{Snapshot()}"),
            };
            return hooks;
        }
    }

    /// <summary>내용물 수명 fixture 의 10.78 몫을 재생한다.</summary>
    [Fact]
    public void ContentLifetimeMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("contentlifetime");
        int patch = 0;
        int cd = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(Columns, row.Length);
            // 이 조각이 옮기지 않은 판본도 자료 구성이 그대로인지 센다
            if (row[0] == CdEdition)
            {
                cd++;
                return true;
            }
            if (row[0] != PatchEdition)
            {
                return true;
            }
            patch++;
            var scene = new Scene(row[2] == "1", row[8], X86Fixture.UInt(row[5]), X86Fixture.UInt(row[3]),
                row[4] == "1", X86Fixture.Int(row[6]), X86Fixture.UInt(row[7]));
            uint result = scene.Apply(row[1]);
            // 원본이 저장한 기대 관찰과 비교한다
            Assert.Equal(X86Fixture.UInt(row[10]), result);
            Assert.Equal(row[12], scene.EffectsText());
            Assert.Equal(row[13], scene.NoticesText());
            // watched 슬롯은 체크섬이 아니라 전체 원본 바이트와 직접 비교한다
            foreach (string item in row[9].Split(';'))
            {
                string[] field = item.Split(':');
                Assert.Equal(field[1], scene.HexSlot(X86Fixture.Int(field[0])));
            }
            // 풀 메타 11개: 크기·빈 수·예측 커서·두 영역 머리/꼬리·전체 체크섬·삭제 깊이
            uint[] expected = row[11].Split(',').Select(X86Fixture.UInt).ToArray();
            Assert.Equal(11, expected.Length);
            uint[] actual =
            [
                (uint)scene.Pool.Capacity,
                scene.Pool.FreeCount,
                scene.Pool.PredictableCursor,
                (uint)scene.Pool.FirstFree(true),
                (uint)scene.Pool.Tail(true),
                (uint)scene.Pool.FirstFree(false),
                (uint)scene.Pool.Tail(false),
                X86Fixture.Adler32(scene.Pool.Bytes),
                scene.Destroy.PreDepth,
                scene.Destroy.PostDepth,
                0,
            ];
            for (int index = 0; index < actual.Length; index++)
            {
                Assert.Equal(expected[index], actual[index]);
            }
            return true;
        });
        Assert.Equal(RowsPerEdition, patch);
        Assert.Equal(RowsPerEdition, cd);
    }
}

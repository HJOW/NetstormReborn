// cpppj FortBadgesGlobalsTests.cpp의 3개 TEST_CASE를 xUnit으로 옮긴 뱃지 전역 장부 검사다. (F2c-6, 2026-11-03)
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 뱃지 전역 장부 검사를 모아 둔 네임스페이스다.
namespace Netstorm.Core.Tests;

/// <summary>
/// 뱃지 28개 기록의 CRT·리셋과 MetaLoader/getter/writer 공유를 원본 x86 기대값으로 대조한다.
/// cpppj <c>FortBadgesGlobalsTests</c>의 3개 TEST_CASE를 xUnit Fact 3개로 옮겼다. (F2c-6, 2026-11-03)
/// </summary>
public sealed class FortBadgesGlobalsTests
{
    /// <summary>
    /// 독립 TSV의 입력과 전체 원문 관측 열을 분리한다.
    /// </summary>
    /// <param name="source">분리할 원문이다.</param>
    /// <param name="separator">구분자다.</param>
    private static string[] GlobalsFields(string source, char separator)
    {
        // 끝의 빈 칸도 남겨 실제 열 수를 보존한다
        return source.Split(separator);
    }

    /// <summary>
    /// 원본에 공급할 명시 profile만 만들고 출력의 기대 초기화는 계산하지 않는다.
    /// </summary>
    /// <param name="profile">입력 profile 번호다.</param>
    private static byte[] GlobalsInitial(uint profile)
    {
        byte[] bytes = new byte[FortBadgesGlobalsShape.GlobalsBytes];
        // type/count/padding도 같은 초기 BYTE 패턴으로 채운다
        for (int index = 0; index < bytes.Length; index++)
        {
            switch (profile)
            {
                case 0: break;
                case 1: bytes[index] = 255; break;
                case 2: bytes[index] = 0xa5; break;
                case 3: bytes[index] = (byte)index; break;
                case 4: bytes[index] = (byte)(255 - index); break;
                case 5: bytes[index] = (byte)(index * 37 + 19); break;
                case 6: bytes[index] = (byte)(((uint)(index / 4) * 0x10003u + 0x80000000u) >> ((index % 4) * 8)); break;
                case 7: bytes[index] = (byte)(1 + index % 255); break;
                default: throw new ArgumentOutOfRangeException(nameof(profile), profile, "뱃지 전역 입력 profile 범위");
            }
        }
        return bytes;
    }

    /// <summary>
    /// 비정렬 little endian DWORD를 현재 기록에서 읽는다.
    /// </summary>
    /// <param name="bytes">읽을 바이트다.</param>
    /// <param name="at">읽을 위치다.</param>
    private static uint GlobalsRead(ReadOnlySpan<byte> bytes, int at = 0)
    {
        uint value = 0;
        // 네 바이트 폭만 합친다
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)bytes[at + index] << (index * 8);
        }
        return value;
    }

    /// <summary>
    /// 명시 stream 필드만 써 실제 writer에 공급한다.
    /// </summary>
    /// <param name="bytes">쓸 바이트다.</param>
    /// <param name="at">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void GlobalsPut(Span<byte> bytes, int at, uint value)
    {
        // 인접 자료는 보존한다
        for (int index = 0; index < 4; index++)
        {
            bytes[at + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>
    /// 물리 계약 오류를 검사 실패와 구별한다.
    /// </summary>
    /// <param name="function">확인할 동작이다.</param>
    private static bool GlobalsRejects(Action function)
    {
        try
        {
            function();
        }
        catch (ArgumentException)
        {
            // C++의 invalid_argument와 같은 물리 계약 거부다
            return true;
        }
        return false;
    }

    /// <summary>
    /// 선택하지 않는 Technology 경계는 성공 noop 대신 명시 거부한다.
    /// </summary>
    /// <param name="tick">BadgeCountRange 진단을 셀 때 호출한다.</param>
    private static FortMetaLoadHooks GlobalsMetaHooks(Action tick)
    {
        // cpp BadgeGlobalsMetaHooks와 같게 Assertion은 BadgeCountRange만 허용하고 센다
        return new FortMetaLoadHooks
        {
            Assertion = reason =>
            {
                Assert.Equal(FortMetaAssertion.BadgeCountRange, reason);
                tick();
            },
            LoadingDepthChanged = _ => throw new InvalidOperationException("패치 Badges가 로딩 깊이를 바꾸면 안 됩니다"),
            TechnologyParent = () => throw new InvalidOperationException("Badges 검사에서 기술 부모 조회 금지"),
            PrepareTypeNames = () => throw new InvalidOperationException("Badges 검사에서 타입 변환 준비 금지"),
            Contents = (_, _, _, _) => throw new InvalidOperationException("Badges 검사에서 기술 내용물 실행 금지"),
            ReleaseTypeConversion = () => throw new InvalidOperationException("Badges 검사에서 타입 변환 반납 금지"),
            RefreshTechnologyUi = () => throw new InvalidOperationException("Badges 검사에서 기술 UI 실행 금지"),
        };
    }

    /// <summary>
    /// 실제 원본 32입력의 초기 0/비0 전체 장부와 두 함수 순서를 전수 비교한다.
    /// cpppj <c>FortBadgesGlobals_IndependentNativeCrtAndReset</c>을 옮겼다.
    /// </summary>
    [Fact]
    public void IndependentNativeCrtAndReset()
    {
        string[][] rows = X86Fixture.Read("fort-badges-globals");
        int compared = 0;
        // 제품 자료를 기대롤 쓰지 않고 뒤의 실제 PE 출력을 비교한다
        foreach (string[] row in rows)
        {
            Assert.Equal(10, row.Length);
            byte[] bytes = GlobalsInitial(uint.Parse(row[2], CultureInfo.InvariantCulture));
            // 원본과 같은 명시 두 함수 순서만 적용한다
            foreach (string stage in GlobalsFields(row[1], '-'))
            {
                if (stage == "crt")
                {
                    RawFortBadgesGlobals.InitializeFortBadgesRecords(bytes);
                }
                else
                {
                    Assert.Equal("reset", stage);
                    RawFortBadgesGlobals.ResetFortBadgeCounts(bytes);
                }
            }
            Assert.True(row[3] == "1" && bytes.AsSpan().SequenceEqual(Convert.FromHexString(row[4])), "Badges 전역 입력: " + row[0]);
            Assert.Equal(0, int.Parse(row[6], CultureInfo.InvariantCulture));
            Assert.True(uint.Parse(row[7], CultureInfo.InvariantCulture) > 0 && uint.Parse(row[8], CultureInfo.InvariantCulture) > 0);
            Assert.Equal(row[1] == "reset" ? 112u : 6152u, uint.Parse(row[9], CultureInfo.InvariantCulture));
            compared++;
        }
        Assert.Equal(32, compared);
    }

    /// <summary>
    /// 새 소유자는 BSS0에서 CRT 한 번이며 span 주소를 보존하고 reset은 count 외 6048BYTE를 바꾸지 않는다.
    /// cpppj <c>FortBadgesGlobals_OwnerAddressAndCountOnlyReset</c>을 옮겼다.
    /// </summary>
    [Fact]
    public void OwnerAddressAndCountOnlyReset()
    {
        FortBadgesGlobalsOwner first = new();
        FortBadgesGlobalsOwner second = new();
        Span<byte> mutableView = first.Records();
        ReadOnlySpan<byte> constant = first.RecordsReadOnly();
        Assert.Equal(FortBadgesGlobalsShape.GlobalsBytes, mutableView.Length);
        Assert.Equal(mutableView.Length, constant.Length);
        // 같은 배열을 빌리므로 쓰기가 읽기 뷰에 보인다 (C++ data() 일치와 같다)
        mutableView[4] ^= 0xff;
        Assert.Equal(mutableView[4], constant[4]);
        mutableView[4] ^= 0xff;
        // 다른 소유자는 별도 배열이므로 쓰기가 건너가지 않는다
        second.Records()[4] ^= 0xff;
        Assert.NotEqual(second.Records()[4], mutableView[4]);
        second.Records()[4] ^= 0xff;
        string[][] rows = X86Fixture.Read("fort-badges-globals");
        Assert.NotEmpty(rows);
        // 최초 BSS0에서 CRT한 원문 한 행을 소유자 생성 결과와 직접 비교한다
        byte[] expected = Convert.FromHexString(rows[0][4]);
        Assert.True(mutableView.SequenceEqual(expected));
        byte[] pattern = GlobalsInitial(7);
        pattern.AsSpan().CopyTo(mutableView);
        byte[] previous = mutableView.ToArray();
        first.ResetCounts();
        // 모든 count 외 바이트를 전부 대조한다
        for (int index = 0; index < mutableView.Length; index++)
        {
            Assert.Equal(index % FortBadgesGlobalsShape.RecordBytes >= FortBadgesGlobalsShape.CountOffset ? 0 : previous[index], mutableView[index]);
        }
        Assert.Equal(previous[4], constant[4]);
        byte[] shortBytes = Enumerable.Repeat((byte)0xa5, FortBadgesGlobalsShape.GlobalsBytes - 1).ToArray();
        byte[] longBytes = Enumerable.Repeat((byte)0xa5, FortBadgesGlobalsShape.GlobalsBytes + 1).ToArray();
        Assert.True(GlobalsRejects(() => RawFortBadgesGlobals.InitializeFortBadgesRecords(shortBytes)));
        Assert.True(GlobalsRejects(() => RawFortBadgesGlobals.ResetFortBadgeCounts(longBytes)));
        // 거부된 버퍼는 그대로 0xa5다
        foreach (byte value in shortBytes)
        {
            Assert.Equal(0xa5, value);
        }
        // 거부된 버퍼는 그대로 0xa5다
        foreach (byte value in longBytes)
        {
            Assert.Equal(0xa5, value);
        }
    }

    /// <summary>
    /// 기존 실제 MetaLoader와 새 getter/writer가 같은 소유자 장부를 직접 사용하고 빈/부분 로드를 보존한다.
    /// cpppj <c>FortBadgesGlobals_SharedMetaLoaderGetterAndWriter</c>를 옮겼다.
    /// C++의 shared_ptr 소유자 대신 같은 6160바이트 배열을 MetaLoader·getter·writer가 공유한다.
    /// </summary>
    [Fact]
    public void SharedMetaLoaderGetterAndWriter()
    {
        byte[] shared = new byte[FortBadgesGlobalsShape.GlobalsBytes];
        RawFortBadgesGlobals.InitializeFortBadgesRecords(shared);
        byte[] original = shared.ToArray();
        RawSidPool pool = new(24000, false, null, OriginalEdition.Patch1078);
        uint depth = 5;
        int assertions = 0;
        FortMetaLoadState state = new();
        RawFortMetaLoad meta = new(pool, shared, () => depth, value => depth = value, state, GlobalsMetaHooks(() => assertions++));
        byte[] load = [2, 71, 7, 0, 9];
        FortMetaStream input = new() { Bytes = new ReadOnlyMemory<byte>(load), Position = 0, End = 5, Present = true };
        meta.LoadBadges(input);
        Assert.Equal(5u, input.Position);
        Assert.Equal(5u, depth);
        Assert.Equal(0, assertions);
        Assert.Equal(7u, RawFortBadgesSave.ReadFortBadgeCount(shared, 71));
        Assert.Equal(9u, RawFortBadgesSave.ReadFortBadgeCount(shared, 0));
        meta.SetBadge(72, 256);
        Assert.Equal(1, assertions);
        Assert.Equal(256u, RawFortBadgesSave.ReadFortBadgeCount(shared, 72));
        FortMetaStream empty = new() { Bytes = ReadOnlyMemory<byte>.Empty, Position = 0, End = 0, Present = false };
        meta.LoadBadges(empty);
        byte[] partial = [1, 71, 8];
        FortMetaStream changed = new() { Bytes = new ReadOnlyMemory<byte>(partial), Position = 0, End = 3, Present = true };
        meta.LoadBadges(changed);
        Assert.Equal(8u, RawFortBadgesSave.ReadFortBadgeCount(shared, 71));
        Assert.Equal(9u, RawFortBadgesSave.ReadFortBadgeCount(shared, 0));
        Assert.Equal(256u, RawFortBadgesSave.ReadFortBadgeCount(shared, 72));
        // setter가 원본 다른 필드/패턴을 건드리지 않는다
        for (int index = 0; index < shared.Length; index++)
        {
            if (index % FortBadgesGlobalsShape.RecordBytes < FortBadgesGlobalsShape.CountOffset)
            {
                Assert.Equal(original[index], shared[index]);
            }
        }
        byte[] fields = new byte[16];
        byte[] buffer = new byte[64];
        GlobalsPut(fields, 0, 77);
        GlobalsPut(fields, 12, 64);
        FortBadgesSaveHooks hooks = new()
        {
            Manager = slot =>
            {
                Assert.Equal(1, slot);
                return 11u;
            },
            Create = (manager, name) =>
            {
                Assert.Equal(11u, manager);
                Assert.Equal("Badges", name);
                return 9u;
            },
            Assertion = _ => throw new InvalidOperationException("충분한 실제 Badges 출력에 진단이 발생했습니다"),
            Count = type => RawFortBadgesSave.ReadFortBadgeCount(shared, type),
            Access = (selected, operation) =>
            {
                Assert.Equal(9u, selected);
                MemStreamHooks memory = new()
                {
                    Buffer = token =>
                    {
                        Assert.Equal(77u, token);
                        return buffer;
                    },
                };
                operation(fields, memory);
            },
        };
        byte[] beforeSave = shared.ToArray();
        RawFortBadgesSave.SaveFortBadges(1, hooks);
        byte[] expected = [3, 0, 9, 71, 8, 72, 0];
        Assert.True(buffer.AsSpan(0, expected.Length).SequenceEqual(expected));
        Assert.Equal(7u, GlobalsRead(fields, 4));
        Assert.Equal(7u, GlobalsRead(fields, 8));
        Assert.True(shared.AsSpan().SequenceEqual(beforeSave));
        // C++의 shared_ptr 해제는 C# 배열 수명과 대응하므로 같은 배열이 살아 있음을 직접 확인한다
        Assert.Equal(8u, RawFortBadgesSave.ReadFortBadgeCount(shared, 71));
        RawFortBadgesGlobals.ResetFortBadgeCounts(shared);
        Assert.Equal(0u, RawFortBadgesSave.ReadFortBadgeCount(shared, 71));
        Assert.Equal(0u, RawFortBadgesSave.ReadFortBadgeCount(shared, 72));
        Assert.Equal(0u, RawFortBadgesSave.ReadFortBadgeCount(shared, 0));
    }
}

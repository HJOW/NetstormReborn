using System.Buffers.Binary;
using Netstorm.Core.Original;
using static Netstorm.Core.Tests.RawPathProcessTests;

// 건설 해체 보상과 다른 실제 취소 환불의 독립 PE 관측을 재생한다.
namespace Netstorm.Core.Tests;

/// <summary>세 원본 PE의 환불·SP 조회·가산과 현재 전역 재조회 경계를 검사한다.</summary>
public sealed class RawConstructionRefundTests
{
    /// <summary>독립 PE의216환불/48잔액/24가산을 사건 순서·원문 BYTE로 대조한다.</summary>
    [Fact]
    public void NativeRefundMoneyAndAddMatch288ObservedCalls()
    {
        int refunds = 0, money = 0, adds = 0;
        // 원본의 배치/비용 부족/거부 관측은 이 최소 모듈의 검증으로 집계하지 않는다.
        foreach (string[] row in Rows("constructionplace-x86.tsv").Where(row => row[0] is "Refund" or "Money" or "Add"))
        {
            uint[] input = row[2].Split(',').Select(U).ToArray(); bool refund = row[0] == "Refund";
            OriginalEdition edition = Edition(row[1]); var pool = new RawSidPool(24000, false, edition: edition);
            var types = Enumerable.Range(0, edition == OriginalEdition.Patch1078 ? 188 : 171).Select(_ => new OriginalTypeRecord()).ToArray();
            var state = new ConstructionPlaceState { LocalPlayer = input[refund ? 2 : 1], BattleShown = row[0] == "Add" ? false : input[refund ? 3 : 2] != 0 };
            if (refund) types[input[0]].Cost = F(input[4].ToString());
            var events = new List<string>();
            var hooks = new ConstructionPlaceHooks
            {
                StoredSp = player => { events.Add($"G:{player}"); return F(input[3].ToString()); },
                AddLocalSp = delta => events.Add($"L:{BitConverter.SingleToUInt32Bits(delta)}"),
                AddPlayerSp = (player, delta) => events.Add($"A:{player}:{BitConverter.SingleToUInt32Bits(delta)}"),
                SendMoney = (player, notice) =>
                {
                    var body = new byte[7]; body[0] = notice.Player; body[1] = notice.Source; body[2] = notice.Kind;
                    BinaryPrimitives.WriteInt32LittleEndian(body.AsSpan(3), notice.Amount); events.Add($"M:{player}:money:{Convert.ToHexString(body).ToLowerInvariant()}");
                },
            };
            var place = new RawConstructionPlace(pool, types, state, hooks);
            if (refund) { place.Refund(input[0], input[1]); refunds++; }
            else if (row[0] == "Money") { Assert.Equal(U(row[5]), BitConverter.SingleToUInt32Bits(place.Money(input[0]))); money++; }
            else { place.AddMoney(input[0], F(input[2].ToString())); adds++; }
            Assert.Equal(row[3], events.Count == 0 ? "-" : string.Join(';', events)); Assert.Equal("-", row[4]);
        }
        Assert.Equal(216, refunds); Assert.Equal(48, money); Assert.Equal(24, adds);
    }
    /// <summary>현재 owner 변경은 가산의 재조회와 너겟 제외에 즉시 반영되고 형식적 저장 사본을 만들지 않는다.</summary>
    [Fact]
    public void RefundReadsCurrentGlobalsAndPreservesDwordTarget()
    {
        var pool = new RawSidPool(24000, false); var types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        types[100].Cost = 37.75f; var nugget = new UIntBox { Value = 154 }; uint local = 257, shown = 0;
        var state = new ConstructionPlaceState { SharedLocalPlayer = () => local, SharedPlayerKnown = () => shown, SharedNuggetType = nugget };
        var events = new List<string>(); var place = new RawConstructionPlace(pool, types, state, new()
        {
            StoredSp = _ => 0, AddLocalSp = amount => { events.Add($"local:{amount}"); shown = 2; }, AddPlayerSp = (player, amount) => events.Add($"other:{player}:{amount}"),
            SendMoney = (player, notice) => { Assert.Equal(257u, player); Assert.Equal((byte)1, notice.Player); Assert.Equal((byte)1, notice.Source); Assert.Equal(37, notice.Amount); events.Add("send"); },
        });
        place.Refund(100, 257); Assert.Equal(new[] { "local:37", "send" }, events); events.Clear();
        place.Refund(100, 257); Assert.Equal(new[] { "send" }, events); events.Clear();
        nugget.Value = 100; place.Refund(100, 257); Assert.Empty(events);
        Assert.Throws<ArgumentOutOfRangeException>(() => place.Refund(188, 1));
    }
    /// <summary>CD는 통지 전용이고 NaN·무한·64비트 상한과 signedDWORD 하위 변환은 관측 계약을 유지한다.</summary>
    [Fact]
    public void ExceptionalCostsAndCdOnlyEffectsRemainExplicit()
    {
        var pool = new RawSidPool(24000, false, edition: OriginalEdition.Cd1072); var types = Enumerable.Range(0, 171).Select(_ => new OriginalTypeRecord()).ToArray();
        var notices = new List<int>(); var cd = new RawConstructionPlace(pool, types, new(), new() { SendMoney = (_, notice) => notices.Add(notice.Amount) });
        types[100].Cost = 12.9f; cd.Refund(100, 3); Assert.Equal(new[] { 12 }, notices);
        // 양수 signedDWORD가 아닌 비용은 CD의 실제 통지를 만들지 않는다.
        foreach (float cost in new[] { float.NaN, float.PositiveInfinity, float.NegativeInfinity, 9223372036854775808f, -9223372036854775808f, -1.5f, 2147483648f }) { types[100].Cost = cost; cd.Refund(100, 3); }
        Assert.Single(notices); Assert.Throws<InvalidOperationException>(() => cd.Money(1)); Assert.Throws<InvalidOperationException>(() => cd.AddMoney(1, 1));
    }
}

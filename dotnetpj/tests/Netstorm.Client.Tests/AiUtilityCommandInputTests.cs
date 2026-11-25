// 원문 일반 입력/FIFO와 세 하위 경계의 뗌·실패 순서를 창 없이 확인한다.
using Netstorm.Client.Input;
using Netstorm.Tests;

namespace Netstorm.Client.Tests;

// AI 직접 명령 입력을 검사하는 클래스다. 2026-11-25 R4-5d AI 직접 명령 이식에서 추가됐다.
// 근거: cpppj tests/AiUtilityCommandInputTests.cpp의 첫 번째 검사 전부다. 두 번째 검사(이름·문자열 고리·FIFO)는
// DN에 아직 없는 GameMenuCommands·ScriptCommand 모듈이 필요해 잇지 않았다.
public sealed class AiUtilityCommandInputTests
{
    /// <summary>실제 선택된 call과 param2 전체 DWORD를 modifier/outside/release마다 비교한다. 2026-11-25 R4-5d AI 직접 명령 이식에서 추가됐다.</summary>
    [Fact]
    public void Native72SelectedParentsAndRelease()
    {
        string[][] rows = X86Fixture.Read("ai-utility-commands");
        int count = 0;
        // 기대 trace는 원문 기계어에서만 읽는 반복문이다.
        foreach (string[] row in rows)
        {
            Assert.Equal(4, row.Length);
            string trace = "";
            var hooks = new AiUtilityCommandHooks();
            hooks.InitialGeysers = () => trace = "Geysers";
            hooks.CrankMoney = () => trace = "Money";
            hooks.Parameter = value => trace = "Param:" + value;
            uint code = X86Fixture.UInt(row[0]);
            int y = unchecked((int)X86Fixture.UInt(row[1]));
            Assert.True(AiUtilityCommandInput.DispatchAiUtilityCommandInput(new InputEvent(code, 37, y), hooks));
            Assert.Equal(row[2], trace.Length == 0 ? "-" : trace);
            count++;
        }

        Assert.Equal(72, count);
        Assert.False(AiUtilityCommandInput.DispatchAiUtilityCommandInput(new InputEvent('X', 0, 0), new AiUtilityCommandHooks()));
        Assert.True(AiUtilityCommandInput.DispatchAiUtilityCommandInput(new InputEvent(AiUtilityCommandInput.BrainCrankMoneyCommand | InputCode.Release, 0, 0), new AiUtilityCommandHooks()));
        Assert.ThrowsAny<Exception>(() => AiUtilityCommandInput.DispatchAiUtilityCommandInput(new InputEvent(AiUtilityCommandInput.CreateInitialGeysersCommand, 0, 0), new AiUtilityCommandHooks()));
        int calls = 0;
        var missing = new AiUtilityCommandHooks();
        missing.Parameter = value =>
        {
            Assert.Equal(0x80000000u, value);
            calls++;
            throw new InvalidOperationException("명시 실제 본체 writer 실패");
        };
        Assert.ThrowsAny<Exception>(() => AiUtilityCommandInput.DispatchAiUtilityCommandInput(new InputEvent(AiUtilityCommandInput.AiParamSetCommand, 123, unchecked((int)0x80000000u)), missing));
        Assert.Equal(1, calls);
    }
}

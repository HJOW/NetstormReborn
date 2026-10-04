namespace Netstorm.Game;

/// <summary>프로그램 진입점</summary>
internal static class Program
{
    /// <summary>게임 창을 만들고 실행한다</summary>
    [STAThread]
    private static void Main()
    {
        using var game = new NetstormGame();
        game.Run();
    }
}

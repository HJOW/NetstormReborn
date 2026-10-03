using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Netstorm.Assets;
using Netstorm.Core.Simulation;

namespace Netstorm.Game;

/// <summary>전투 규칙의 캐논·재성장·방어선을 원본 셰이프 프레임으로 표시한다.</summary>
internal sealed partial class FortMapViewer
{
    /// <summary>썬 바리케이트 발자국 중심에서 기둥 머리까지의 논리 화면 거리. TEST01 녹화에 맞춘 위치다.</summary>
    private static readonly Vector2 FenceHeadOffset = new(-12, -27);

    /// <summary>고정 그림 대신 현재 전투 상태에서 그릴 타입·프레임. 이동형 유닛의 걷기는 기존 처리에 맡긴다.</summary>
    private (TypeInfo Type, int Frame)? CombatSprite(GameEntity entity)
    {
        if (entity.IsRegenerating && _knowledgeTypes.Find("growingRainBlocker") is { } growing)
            return (growing, _session.IceGrowthFrame(entity));
        if (CannonAnimation.IsCannon(entity.Type))
            return (entity.Type, CannonAnimation.Frame(entity, _session.Tick, _session.TicksPerSecond));
        return null;
    }

    /// <summary>기둥 머리 사이를 원본 fenceShield의 가로·세로 그림으로 채운다. 기둥보다 먼저 그려 끝점을 가린다.</summary>
    private void DrawSunForceFields(SpriteBatch batch, Vector2 center)
    {
        if (_knowledgeTypes.Find("fenceShield") is not { } type) return;
        // 규칙에서 계산한 실제 연결만 표시해 적 기둥·거리·기둥 제거 결과와 일치시킨다.
        foreach (SunForceField field in _session.SunForceFields)
        {
            bool horizontal = field.StartY == field.EndY;
            int frame = type.Definition.Frames.Find(horizontal ? 'K' : 'J', TypeFrameTable.DefaultVariant, 0);
            var sprite = GetTexture(type.LoadIndex, frame, _playerColors.GetValueOrDefault(field.Owner));
            if (sprite is not { } image) continue;
            Vector2 start = CellCenterScreen(field.StartX, field.StartY, center) + FenceHeadOffset * _zoom;
            Vector2 end = CellCenterScreen(field.EndX, field.EndY, center) + FenceHeadOffset * _zoom;
            int cells = (int)(horizontal ? field.EndX - field.StartX : field.EndY - field.StartY);
            // 한 칸짜리 원본 그림을 이어 붙여 해상도·화면비가 달라도 같은 굵기를 유지한다.
            for (int cell = 0; cell < cells; cell++)
            {
                Vector2 position = Vector2.Lerp(start, end, cell / (float)cells);
                if (horizontal) position.Y -= image.Texture.Height * _zoom / 2;
                else position.X -= image.Texture.Width * _zoom / 2;
                Color color = field.Owner == TestPlayer ? Color.Turquoise : field.Owner == 2 ? Color.Salmon : Color.White;
                batch.Draw(image.Texture, position, null, color, 0, Vector2.Zero, _zoom, SpriteEffects.None, 0);
            }
        }
    }

    /// <summary>아이스·썬더 캐논의 원본 방향별 탄을 표시한다. 다른 탄은 기존 표시를 사용한다.</summary>
    private bool DrawCannonProjectile(SpriteBatch batch, CombatShot shot, Vector2 position)
    {
        string? name = shot.AttackerType?.ToLowerInvariant() switch
        {
            "raincannon" => "rainCannonMissile", "thundercannon" => "thunderCannonMissile", _ => null,
        };
        if (name == null || _knowledgeTypes.Find(name) is not { } type) return false;
        int direction = shot.EndY < shot.StartY ? 0 : shot.EndX > shot.StartX ? 1 : shot.EndY > shot.StartY ? 2 : 3;
        int frame = name == "thunderCannonMissile"
            ? direction * 3 + (int)((_session.Tick - shot.FiredTick) / 2 % 3)
            // 얼음 탄의 28방향 그림은 북→서→남→동 순서이고, 썬더 탄의 네 방향은 북→동→남→서다.
            : ((4 - direction) % 4) * 7;
        DrawSprite(batch, type.LoadIndex, frame, position - new Vector2(0, 20 * _zoom));
        return true;
    }
}

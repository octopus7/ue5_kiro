using System.Numerics;

namespace SimpleTcpServer
{
    public class Character
    {
        public string Id { get; set; }
        public ushort UserId { get; set; } // 16비트 부호없는 정수형 식별자
        public Vector2 Position { get; set; }
        public Vector2 TargetPosition { get; set; }
        public bool IsMoving { get; set; }
        public float Speed { get; set; } = 100.0f; // units per second
        public DateTime LastUpdateTime { get; set; }

        public Character(string id, ushort userId)
        {
            Id = id;
            UserId = userId;
            Position = Vector2.Zero;
            TargetPosition = Vector2.Zero;
            IsMoving = false;
            LastUpdateTime = DateTime.Now;
        }

        public void StartMovement(Vector2 startPos, Vector2 targetPos)
        {
            Position = startPos;
            TargetPosition = targetPos;
            IsMoving = true;
            LastUpdateTime = DateTime.Now;
        }

        public void Update()
        {
            if (!IsMoving) return;

            var now = DateTime.Now;
            var deltaTime = (float)(now - LastUpdateTime).TotalSeconds;
            LastUpdateTime = now;

            var direction = Vector2.Normalize(TargetPosition - Position);
            var distance = Vector2.Distance(Position, TargetPosition);
            var moveDistance = Speed * deltaTime;

            if (moveDistance >= distance)
            {
                Position = TargetPosition;
                IsMoving = false;
            }
            else
            {
                Position += direction * moveDistance;
            }
        }
    }
}
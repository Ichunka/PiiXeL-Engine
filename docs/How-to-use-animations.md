# How to Use Animations

## Asset Pipeline

```
Texture (.png)
    ↓
SpriteSheet (.spritesheet)    ← Define frames with positions/pivots
    ↓
AnimationClip (.animclip)     ← Sequence of frames + durations
    ↓
AnimatorController (.animcontroller)  ← State machine
    ↓
Animator Component            ← Attached to entity
```

## 1. Create Sprite Sheet

**Editor → Content Browser → Right Click → New → Sprite Sheet**

Define frames:
- Position (x, y)
- Size (width, height)
- Pivot point (origin)

## 2. Create Animation Clip

**Editor → Content Browser → Right Click → New → Animation Clip**

Add sprite frames:
- Select sprite from sheet
- Set frame duration (seconds)
- Set wrap mode (Once/Loop/PingPong)

## 3. Create Animator Controller

**Editor → Content Browser → Right Click → New → Animator Controller**

### Add States
```
Idle    → Loop idle animation
Walk    → Loop walk animation
Jump    → Play once jump animation
```

### Add Parameters
```
Speed (Float)       → Control walk/idle transition
IsGrounded (Bool)   → Enable jump
Jump (Trigger)      → Fire jump event
```

### Add Transitions
```
Idle → Walk
  Condition: Speed > 0.1

Walk → Idle
  Condition: Speed < 0.1

Idle → Jump
  Condition: Jump (trigger)

Walk → Jump
  Condition: Jump (trigger)
```

## 4. Add Animator Component

Select entity → Add Component → Animator

- Assign AnimatorController
- Set playback speed (default: 1.0)

## 5. Control from Script

```cpp
class Player : public PiiXeL::ScriptComponent {
private:
    std::optional<PiiXeL::AnimatorHandle> m_Animator;
    std::optional<PiiXeL::RigidBodyHandle> m_RigidBody;

    void OnStart() override {
        m_Animator = GetHandle<PiiXeL::Animator>();
        m_RigidBody = GetHandle<PiiXeL::RigidBody2D>();
    }

    void OnUpdate(float deltaTime) override {
        if (!m_Animator || !m_RigidBody) return;

        Vector2 velocity = m_RigidBody->GetVelocity();

        // Update animation parameters
        m_Animator->SetFloat("Speed", std::abs(velocity.x));
        m_Animator->SetBool("IsGrounded", CheckGrounded());

        if (IsKeyPressed(KEY_SPACE) && CheckGrounded()) {
            m_Animator->SetTrigger("Jump");
            m_RigidBody->ApplyImpulse(Vector2{0.0f, -500.0f});
        }
    }

    bool CheckGrounded() {
        // Ground check logic
        return true;
    }
};
```

## AnimatorHandle API

```cpp
// Set parameters
animator->SetFloat("Speed", 5.0f);
animator->SetInt("State", 2);
animator->SetBool("IsGrounded", true);
animator->SetTrigger("Jump");  // Fires once, auto-resets

// Read parameters
float speed = animator->GetFloat("Speed");
int state = animator->GetInt("State");
bool grounded = animator->GetBool("IsGrounded");

// Playback control
animator->Play();
animator->Pause();
animator->Stop();
animator->SetSpeed(2.0f);  // 2x speed

// Query state
std::string currentState = animator->GetCurrentState();
bool isPlaying = animator->IsPlaying();
float stateTime = animator->GetStateTime();
```

## Edit Mode Preview

In **Edit Mode**, animator shows **first frame** of default state.

In **Play Mode**, animator runs state machine normally.

## Tips

- Use **triggers** for one-shot events (jump, attack)
- Use **floats** for smooth transitions (speed, blend)
- Use **bools** for binary states (grounded, attacking)
- Set **transition duration** for smooth blending
- Create transitions from multiple states for common animations (death, hit)

## Example: Character States

```
States:
  Idle        → Loop, priority: 0
  Walk        → Loop, priority: 1
  Run         → Loop, priority: 1
  Jump        → Once, priority: 2
  Fall        → Loop, priority: 2
  Land        → Once, priority: 3

Parameters:
  Speed (Float)         → 0.0 - 10.0
  VerticalVel (Float)   → -100.0 - 100.0
  IsGrounded (Bool)
  Jump (Trigger)

Transitions:
  Idle → Walk:        Speed > 0.1
  Walk → Idle:        Speed < 0.1
  Walk → Run:         Speed > 5.0
  Run → Walk:         Speed < 5.0
  Grounded → Jump:    Jump (trigger)
  Jump → Fall:        VerticalVel > 0
  Fall → Land:        IsGrounded = true
  Land → Idle:        Animation finished
```

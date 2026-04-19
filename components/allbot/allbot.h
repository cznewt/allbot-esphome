#pragma once

#include <map>
#include <string>
#include <vector>

#include "esphome/core/component.h"
#include "esphome/components/output/float_output.h"

namespace esphome {
namespace allbot {

class AllbotServo {
 public:
  void configure(output::FloatOutput *out, bool flipped, int offset, int initial_angle,
                 uint16_t min_us, uint16_t max_us) {
    this->output_ = out;
    this->flipped_ = flipped;
    this->offset_ = offset;
    this->angle_ = initial_angle;
    this->to_angle_ = initial_angle;
    this->cur_angle_ = initial_angle;
    this->min_us_ = min_us;
    this->max_us_ = max_us;
  }

  bool flipped() const { return this->flipped_; }
  int offset() const { return this->offset_; }
  int angle() const { return this->angle_; }

  void write(int angle);
  void move(int angle) { this->to_angle_ = angle; }
  void reset() { this->to_angle_ = this->angle_; }
  void prepare(int speed_ms);
  bool tick();

 protected:
  output::FloatOutput *output_{nullptr};
  bool flipped_{false};
  int offset_{0};
  uint16_t min_us_{544};
  uint16_t max_us_{2400};

  int angle_{45};
  int to_angle_{45};
  double cur_angle_{45.0};
  double step_angle_{0.0};
  int step_{0};
  int steps_{0};
};

struct AllbotMove {
  size_t servo_index;
  int angle;
};

struct AllbotStep {
  std::vector<AllbotMove> moves;
  uint32_t duration_ms;
};

using AllbotAnimation = std::vector<AllbotStep>;

class AllbotComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void add_servo(output::FloatOutput *out, bool flipped, int offset, int initial_angle,
                 uint16_t min_us, uint16_t max_us);
  void add_action(const std::string &name, AllbotAnimation animation);

  void move(size_t servo_index, int angle);
  void animate(uint32_t duration_ms);
  void run_action(const std::string &name);
  void stop();
  bool is_busy() const { return this->state_ != State::IDLE; }

 protected:
  enum class State { IDLE, ANIMATING, RUNNING_ACTION };

  void start_step_(const AllbotStep &step);
  void advance_step_();

  std::vector<AllbotServo> servos_;
  std::map<std::string, AllbotAnimation> actions_;

  State state_{State::IDLE};
  std::string current_action_;
  size_t step_index_{0};
  uint32_t last_tick_ms_{0};
  uint32_t step_started_ms_{0};
  uint32_t step_duration_ms_{0};
  bool step_has_moves_{false};
};

}  // namespace allbot
}  // namespace esphome

#include "allbot.h"

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace allbot {

static const char *const TAG = "allbot";

void AllbotServo::write(int angle) {
  this->angle_ = angle;
  int out_angle = this->flipped_ ? 180 - angle : angle;
  out_angle += this->flipped_ ? -this->offset_ : this->offset_;

  if (out_angle < 0)
    out_angle = 0;
  if (out_angle > 180)
    out_angle = 180;

  float pulse_us = this->min_us_ + (this->max_us_ - this->min_us_) * (out_angle / 180.0f);
  float level = pulse_us / 20000.0f;
  if (this->output_ != nullptr)
    this->output_->set_level(level);
}

void AllbotServo::prepare(int speed_ms) {
  int angle_diff = this->to_angle_ - this->angle_;
  if (angle_diff < 0)
    angle_diff = -angle_diff;

  if (angle_diff == 0 || speed_ms <= 0) {
    this->step_ = 0;
    this->steps_ = 0;
    return;
  }

  this->step_angle_ = (double) angle_diff / (double) speed_ms;
  this->cur_angle_ = this->angle_;
  this->step_ = 0;
  this->steps_ = angle_diff / this->step_angle_;

  if (this->to_angle_ < this->angle_)
    this->step_angle_ *= -1.0;
}

bool AllbotServo::tick() {
  if (this->step_ < this->steps_) {
    this->cur_angle_ += this->step_angle_;
    this->write((int) this->cur_angle_);
    this->step_++;
  }
  return this->step_ >= this->steps_;
}

void AllbotComponent::add_servo(output::FloatOutput *out, bool flipped, int offset,
                                 int initial_angle, uint16_t min_us, uint16_t max_us) {
  this->servos_.emplace_back();
  this->servos_.back().configure(out, flipped, offset, initial_angle, min_us, max_us);
}

void AllbotComponent::add_action(const std::string &name, AllbotAnimation animation) {
  this->actions_[name] = std::move(animation);
}

void AllbotComponent::setup() {
  for (auto &s : this->servos_)
    s.write(s.angle());
}

void AllbotComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "ALLBOT:");
  ESP_LOGCONFIG(TAG, "  Servos: %u", (unsigned) this->servos_.size());
  for (size_t i = 0; i < this->servos_.size(); i++) {
    auto &s = this->servos_[i];
    ESP_LOGCONFIG(TAG, "    [%u] flipped=%s offset=%d initial=%d", (unsigned) i,
                  YESNO(s.flipped()), s.offset(), s.angle());
  }
  ESP_LOGCONFIG(TAG, "  Actions: %u", (unsigned) this->actions_.size());
  for (auto &kv : this->actions_) {
    ESP_LOGCONFIG(TAG, "    - %s (%u steps)", kv.first.c_str(), (unsigned) kv.second.size());
  }
}

void AllbotComponent::move(size_t servo_index, int angle) {
  if (servo_index >= this->servos_.size())
    return;
  this->servos_[servo_index].move(angle);
}

void AllbotComponent::animate(uint32_t duration_ms) {
  for (auto &s : this->servos_)
    s.prepare((int) duration_ms);
  this->state_ = State::ANIMATING;
  this->step_started_ms_ = millis();
  this->step_duration_ms_ = duration_ms;
  this->step_has_moves_ = true;
  this->last_tick_ms_ = this->step_started_ms_;
}

void AllbotComponent::run_action(const std::string &name) {
  auto it = this->actions_.find(name);
  if (it == this->actions_.end()) {
    ESP_LOGW(TAG, "run_action: unknown action '%s'", name.c_str());
    return;
  }
  if (it->second.empty()) {
    ESP_LOGW(TAG, "run_action: action '%s' has no steps", name.c_str());
    return;
  }
  this->current_action_ = name;
  this->step_index_ = 0;
  this->state_ = State::RUNNING_ACTION;
  this->start_step_(it->second[0]);
}

void AllbotComponent::stop() {
  this->state_ = State::IDLE;
  this->current_action_.clear();
  this->step_index_ = 0;
  for (auto &s : this->servos_)
    s.reset();
}

void AllbotComponent::start_step_(const AllbotStep &step) {
  for (auto &m : step.moves) {
    if (m.servo_index < this->servos_.size())
      this->servos_[m.servo_index].move(m.angle);
  }
  this->step_has_moves_ = !step.moves.empty();
  this->step_duration_ms_ = step.duration_ms;
  this->step_started_ms_ = millis();
  this->last_tick_ms_ = this->step_started_ms_;
  if (this->step_has_moves_) {
    for (auto &s : this->servos_)
      s.prepare((int) step.duration_ms);
  }
}

void AllbotComponent::advance_step_() {
  auto it = this->actions_.find(this->current_action_);
  if (it == this->actions_.end()) {
    this->stop();
    return;
  }
  this->step_index_++;
  if (this->step_index_ >= it->second.size()) {
    this->state_ = State::IDLE;
    this->current_action_.clear();
    this->step_index_ = 0;
    return;
  }
  this->start_step_(it->second[this->step_index_]);
}

void AllbotComponent::loop() {
  if (this->state_ == State::IDLE)
    return;

  uint32_t now = millis();

  if (!this->step_has_moves_) {
    if (now - this->step_started_ms_ >= this->step_duration_ms_) {
      if (this->state_ == State::RUNNING_ACTION) {
        this->advance_step_();
      } else {
        this->state_ = State::IDLE;
      }
    }
    return;
  }

  uint32_t delta = now - this->last_tick_ms_;
  if (delta == 0)
    return;
  this->last_tick_ms_ = now;

  bool done = true;
  for (uint32_t i = 0; i < delta; i++) {
    done = true;
    for (auto &s : this->servos_)
      done &= s.tick();
    if (done)
      break;
  }

  if (done) {
    if (this->state_ == State::RUNNING_ACTION) {
      this->advance_step_();
    } else {
      this->state_ = State::IDLE;
    }
  }
}

}  // namespace allbot
}  // namespace esphome

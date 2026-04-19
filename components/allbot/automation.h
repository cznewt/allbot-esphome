#pragma once

#include "allbot.h"
#include "esphome/core/automation.h"

namespace esphome {
namespace allbot {

template<typename... Ts> class MoveAction : public Action<Ts...> {
 public:
  explicit MoveAction(AllbotComponent *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(uint32_t, servo)
  TEMPLATABLE_VALUE(int, angle)

  void play(Ts... x) override {
    this->parent_->move((size_t) this->servo_.value(x...), this->angle_.value(x...));
  }

 protected:
  AllbotComponent *parent_;
};

template<typename... Ts> class AnimateAction : public Action<Ts...> {
 public:
  explicit AnimateAction(AllbotComponent *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(uint32_t, duration)

  void play(Ts... x) override { this->parent_->animate(this->duration_.value(x...)); }

 protected:
  AllbotComponent *parent_;
};

template<typename... Ts> class RunActionAction : public Action<Ts...> {
 public:
  explicit RunActionAction(AllbotComponent *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(std::string, action)

  void play(Ts... x) override { this->parent_->run_action(this->action_.value(x...)); }

 protected:
  AllbotComponent *parent_;
};

template<typename... Ts> class StopAction : public Action<Ts...> {
 public:
  explicit StopAction(AllbotComponent *parent) : parent_(parent) {}
  void play(Ts... x) override { this->parent_->stop(); }

 protected:
  AllbotComponent *parent_;
};

template<typename... Ts> class BusyCondition : public Condition<Ts...> {
 public:
  explicit BusyCondition(AllbotComponent *parent) : parent_(parent) {}
  bool check(Ts... x) override { return this->parent_->is_busy(); }

 protected:
  AllbotComponent *parent_;
};

}  // namespace allbot
}  // namespace esphome

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import output
from esphome.const import (
    CONF_DURATION,
    CONF_ID,
    CONF_OFFSET,
    CONF_OUTPUT,
)

CONF_ACTION = "action"

CODEOWNERS = ["@cznewt"]
AUTO_LOAD = ["output"]

allbot_ns = cg.esphome_ns.namespace("allbot")
AllbotComponent = allbot_ns.class_("AllbotComponent", cg.Component)
MoveAction = allbot_ns.class_("MoveAction", automation.Action)
AnimateAction = allbot_ns.class_("AnimateAction", automation.Action)
RunActionAction = allbot_ns.class_("RunActionAction", automation.Action)
StopAction = allbot_ns.class_("StopAction", automation.Action)
BusyCondition = allbot_ns.class_("BusyCondition", automation.Condition)

CONF_ACTIONS = "actions"
CONF_ANGLE = "angle"
CONF_FLIPPED = "flipped"
CONF_INITIAL_ANGLE = "initial_angle"
CONF_MAX_PULSE_WIDTH = "max_pulse_width"
CONF_MIN_PULSE_WIDTH = "min_pulse_width"
CONF_MOVES = "moves"
CONF_SERVO = "servo"
CONF_SERVOS = "servos"

ANGLE_RANGE = cv.int_range(min=0, max=180)

SERVO_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ID): cv.valid_name,
        cv.Required(CONF_OUTPUT): cv.use_id(output.FloatOutput),
        cv.Optional(CONF_INITIAL_ANGLE, default=45): ANGLE_RANGE,
        cv.Optional(CONF_FLIPPED, default=False): cv.boolean,
        cv.Optional(CONF_OFFSET, default=0): cv.int_range(min=-90, max=90),
        cv.Optional(
            CONF_MIN_PULSE_WIDTH, default="544us"
        ): cv.positive_time_period_microseconds,
        cv.Optional(
            CONF_MAX_PULSE_WIDTH, default="2400us"
        ): cv.positive_time_period_microseconds,
    }
)

MOVE_ITEM_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_SERVO): cv.valid_name,
        cv.Required(CONF_ANGLE): ANGLE_RANGE,
    }
)

STEP_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_DURATION): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_MOVES, default=[]): cv.ensure_list(MOVE_ITEM_SCHEMA),
    }
)


def _validate_actions(value):
    value = cv.Schema({cv.valid_name: cv.ensure_list(STEP_SCHEMA)})(value)
    return value


def _validate_unique_servo_ids(config):
    names = [s[CONF_ID] for s in config[CONF_SERVOS]]
    if len(set(names)) != len(names):
        raise cv.Invalid("servo ids must be unique")
    index = {n: i for i, n in enumerate(names)}
    for act_name, steps in config[CONF_ACTIONS].items():
        for i, step in enumerate(steps):
            for m in step[CONF_MOVES]:
                if m[CONF_SERVO] not in index:
                    raise cv.Invalid(
                        f"action '{act_name}' step {i}: unknown servo '{m[CONF_SERVO]}'"
                    )
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(AllbotComponent),
            cv.Required(CONF_SERVOS): cv.All(
                cv.ensure_list(SERVO_SCHEMA), cv.Length(min=1)
            ),
            cv.Optional(CONF_ACTIONS, default={}): _validate_actions,
        }
    ).extend(cv.COMPONENT_SCHEMA),
    _validate_unique_servo_ids,
)


def _render_animation(steps, servo_index):
    step_exprs = []
    for step in steps:
        move_exprs = [
            f"esphome::allbot::AllbotMove{{{servo_index[m[CONF_SERVO]]}, {m[CONF_ANGLE]}}}"
            for m in step[CONF_MOVES]
        ]
        moves_literal = (
            "std::vector<esphome::allbot::AllbotMove>{"
            + ", ".join(move_exprs)
            + "}"
        )
        duration_ms = int(step[CONF_DURATION].total_milliseconds)
        step_exprs.append(
            f"esphome::allbot::AllbotStep{{{moves_literal}, {duration_ms}}}"
        )
    return "esphome::allbot::AllbotAnimation{" + ", ".join(step_exprs) + "}"


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    servo_index = {}
    for i, s in enumerate(config[CONF_SERVOS]):
        servo_index[s[CONF_ID]] = i
        out = await cg.get_variable(s[CONF_OUTPUT])
        cg.add(
            var.add_servo(
                out,
                s[CONF_FLIPPED],
                s[CONF_OFFSET],
                s[CONF_INITIAL_ANGLE],
                s[CONF_MIN_PULSE_WIDTH].total_microseconds,
                s[CONF_MAX_PULSE_WIDTH].total_microseconds,
            )
        )

    for name, steps in config[CONF_ACTIONS].items():
        rendered = _render_animation(steps, servo_index)
        cg.add(
            cg.RawExpression(
                f'{var}->add_action("{name}", {rendered})'
            )
        )


MOVE_ACTION_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(AllbotComponent),
        cv.Required(CONF_SERVO): cv.templatable(cv.positive_int),
        cv.Required(CONF_ANGLE): cv.templatable(ANGLE_RANGE),
    }
)


@automation.register_action("allbot.move", MoveAction, MOVE_ACTION_SCHEMA)
async def allbot_move_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    servo = await cg.templatable(config[CONF_SERVO], args, cg.uint32)
    cg.add(var.set_servo(servo))
    angle = await cg.templatable(config[CONF_ANGLE], args, int)
    cg.add(var.set_angle(angle))
    return var


ANIMATE_ACTION_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(AllbotComponent),
        cv.Required(CONF_DURATION): cv.templatable(
            cv.positive_time_period_milliseconds
        ),
    }
)


@automation.register_action("allbot.animate", AnimateAction, ANIMATE_ACTION_SCHEMA)
async def allbot_animate_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    duration = await cg.templatable(
        config[CONF_DURATION], args, cg.uint32, to_exp=lambda x: x.total_milliseconds
    )
    cg.add(var.set_duration(duration))
    return var


RUN_ACTION_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(AllbotComponent),
        cv.Required(CONF_ACTION): cv.templatable(cv.string_strict),
    }
)


@automation.register_action("allbot.run_action", RunActionAction, RUN_ACTION_SCHEMA)
async def allbot_run_action_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    action_name = await cg.templatable(config[CONF_ACTION], args, cg.std_string)
    cg.add(var.set_action(action_name))
    return var


STOP_ACTION_SCHEMA = cv.Schema(
    {cv.GenerateID(): cv.use_id(AllbotComponent)}
)


@automation.register_action("allbot.stop", StopAction, STOP_ACTION_SCHEMA)
async def allbot_stop_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, parent)


BUSY_CONDITION_SCHEMA = cv.Schema(
    {cv.GenerateID(): cv.use_id(AllbotComponent)}
)


@automation.register_condition("allbot.is_busy", BusyCondition, BUSY_CONDITION_SCHEMA)
async def allbot_is_busy_to_code(config, condition_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(condition_id, template_arg, parent)

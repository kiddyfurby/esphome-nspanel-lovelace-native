#include "cards.h"

#include "card_base.h"
#include "config.h"
#include "entity.h"
#include "helpers.h"
#include "page_items.h"
#include "translations.h"
#include "types.h"
#include <string>
#include <memory>

namespace esphome {
namespace nspanel_lovelace {

/*
 * =============== GridCard ===============
 */

void GridCard::accept(PageVisitor& visitor) { visitor.visit(*this); }

/*
 * =============== EntitiesCard ===============
 */

void EntitiesCard::accept(PageVisitor& visitor) { visitor.visit(*this); }

/*
 * =============== QRCard ===============
 */

void QRCard::accept(PageVisitor& visitor) { visitor.visit(*this); }

std::string &QRCard::render(std::string &buffer) {
  buffer.assign(this->get_render_instruction())
      .append(1, SEPARATOR)
      .append(this->get_title())
      .append(1, SEPARATOR);
  
  this->render_nav(buffer).append(1, SEPARATOR);

  buffer.append(this->qr_text_);

  for (auto& item : this->items_) {
    buffer.append(1, SEPARATOR).append(item->render());
  }

  return buffer;
}

/*
 * =============== AlarmCard ===============
 */

AlarmCard::AlarmCard(
  const std::string &uuid, const std::shared_ptr<Entity> &alarm_entity) :
    Card(page_type::cardAlarm, uuid),
    alarm_entity_(alarm_entity),
    show_keypad_(true), status_icon_flashing_(false) {
  alarm_entity_->add_subscriber(this);
  this->status_icon_ = std::unique_ptr<AlarmIconItem>(
    new AlarmIconItem(std::string(uuid).append("_s"), u8"\uE99D", 3302)); //shield-off, green
  this->disarm_button_ = std::unique_ptr<AlarmButtonItem>(
    new AlarmButtonItem(std::string(uuid).append("_d"),
      button_type::disarm, "Disarm"));
}
AlarmCard::AlarmCard(
  const std::string &uuid, const std::shared_ptr<Entity> &alarm_entity,
  const std::string &title) :
    Card(page_type::cardAlarm, uuid, title),
    alarm_entity_(alarm_entity),
    show_keypad_(true),status_icon_flashing_(false) {
  alarm_entity_->add_subscriber(this);
  this->status_icon_ = std::unique_ptr<AlarmIconItem>(
    new AlarmIconItem(std::string(uuid).append("_s"), u8"\uE99D", 3302)); //shield-off, green
  this->disarm_button_ = std::unique_ptr<AlarmButtonItem>(
    new AlarmButtonItem(std::string(uuid).append("_d"),
      button_type::disarm, "Disarm"));
}
AlarmCard::AlarmCard(
    const std::string &uuid, const std::shared_ptr<Entity> &alarm_entity,
    const std::string &title, const uint16_t sleep_timeout) :
    Card(page_type::cardAlarm, uuid, title, sleep_timeout),
    alarm_entity_(alarm_entity),
    show_keypad_(true),status_icon_flashing_(false) {
  alarm_entity_->add_subscriber(this);
  this->status_icon_ = std::unique_ptr<AlarmIconItem>(
    new AlarmIconItem(std::string(uuid).append("_s"), u8"\uE99D", 3302)); //shield-off, green
  this->disarm_button_ = std::unique_ptr<AlarmButtonItem>(
    new AlarmButtonItem(std::string(uuid).append("_d"), 
      button_type::disarm, "Disarm"));
}

AlarmCard::~AlarmCard() {
  alarm_entity_->remove_subscriber(this);
}

void AlarmCard::accept(PageVisitor& visitor) { visitor.visit(*this); }

bool AlarmCard::set_arm_button(
    alarm_arm_action action, const std::string &display_name) {
  if (this->items_.size() >= 4) {
    return false;
  }

  const char *action_type = nullptr;
  switch(action) {
    case alarm_arm_action::arm_home:
      action_type = button_type::armHome;
      break;
    case alarm_arm_action::arm_away:
      action_type = button_type::armAway;
      break;
    case alarm_arm_action::arm_night:
      action_type = button_type::armNight;
      break;
    case alarm_arm_action::arm_vacation:
      action_type = button_type::armVacation;
      break;
  }

  this->items_.push_back(
    std::unique_ptr<AlarmButtonItem>(
      new AlarmButtonItem(
        std::string(this->uuid_).append(1, '_').append(action_type), 
        action_type, display_name)));
  return true;
}

void AlarmCard::set_disarm_button(const std::string &display_name) {
  this->disarm_button_.reset();
  this->disarm_button_ = std::unique_ptr<AlarmButtonItem>(
    new AlarmButtonItem(std::string(this->uuid_).append("_d"),
      button_type::disarm, display_name));
}

void AlarmCard::on_entity_state_change(const std::string &state) {
  this->status_icon_flashing_ = false;

  if (state == alarm_entity_state::disarmed || 
      state == generic_type::unknown) {
    this->status_icon_->reset_icon_color(); //green
    this->status_icon_->reset_icon_value(); //shield-off
  } else if (state == alarm_entity_state::triggered) {
    this->status_icon_->set_icon_color(0xE243); //red
    this->status_icon_->set_icon_value(u8"\uE09D"); //bell-ring
    this->status_icon_flashing_ = true;
  } else if (state == alarm_entity_state::armed_home) {
    this->status_icon_->set_icon_color(0xE243); //red
    this->status_icon_->set_icon_value(u8"\uE689"); //shield-home
  } else if (state == alarm_entity_state::armed_away) {
    this->status_icon_->set_icon_color(0xE243); //red
    this->status_icon_->set_icon_value(u8"\uE99C"); //shield-lock
  } else if (state == alarm_entity_state::armed_night) {
    this->status_icon_->set_icon_color(0xE243); //red
    this->status_icon_->set_icon_value(u8"\uF827"); //shield-moon (was E593:weather-night)
  } else if (state == alarm_entity_state::armed_vacation) {
    this->status_icon_->set_icon_color(0xE243); //red
    this->status_icon_->set_icon_value(u8"\uE6BA"); //shield-airplane
  } else if (state == alarm_entity_state::armed_custom_bypass) {
    this->status_icon_->set_icon_color(0xE243); //red
    this->status_icon_->set_icon_value(u8"\uE497"); //shield
  } else if (state == alarm_entity_state::arming || 
      state == alarm_entity_state::pending) {
    this->status_icon_->set_icon_color(0xED80); //orange
    this->status_icon_->set_icon_value(u8"\uE497"); //shield
    this->status_icon_flashing_ = true;
  } else {
    this->status_icon_->set_icon_color(38066u); //grey
    this->status_icon_->set_icon_value(u8"\uE624"); //help-circle-outline
  }
}

void AlarmCard::on_entity_attribute_change(ha_attr_type attr, const std::string &value) {
  if (attr == ha_attr_type::code_arm_required) {
    this->set_show_keypad(value != generic_type::off);
  }
}

std::string &AlarmCard::render(std::string &buffer) {
  buffer.assign(this->get_render_instruction())
      .append(1, SEPARATOR)
      .append(this->get_title())
      .append(1, SEPARATOR);
  
  this->render_nav(buffer).append(1, SEPARATOR);

  buffer.append(this->alarm_entity_->get_entity_id());

  if (this->alarm_entity_->is_state(generic_type::unknown) ||
      this->alarm_entity_->is_state(alarm_entity_state::disarmed)) {
    for (auto& item : this->items_) {
      buffer.append(1, SEPARATOR).append(item->render());
    }
    if (this->items_.size() < 4) {
      buffer.append(2 * (4 - this->items_.size()), SEPARATOR);
    }
  } else {
    buffer.append(1, SEPARATOR).append(this->disarm_button_->render());
    buffer.append(2 * 3, SEPARATOR);
  }

  buffer.append(1, SEPARATOR).append(this->status_icon_->render());

  buffer.append(1, SEPARATOR)
    .append(this->show_keypad_ ? 
      generic_type::enable : generic_type::disable);

  buffer.append(1, SEPARATOR)
    .append(this->status_icon_flashing_ ? 
      generic_type::enable : generic_type::disable);
  
  // todo: 
  //   if "open_sensors" in entity.attributes and entity.attributes.open_sensors is not None:
  // if (this->info_icon_ && /* ?? */) {
  //   buffer.append(1, SEPARATOR).append(this->info_icon_->render());
  // }

  return buffer;
}

/*
 * =============== ThermoCard ===============
 */

ThermoCard::ThermoCard(const std::string &uuid,
    const std::shared_ptr<Entity> &thermo_entity) :
    Card(page_type::cardThermo, uuid),
    thermo_entity_(thermo_entity) {
  this->configure_temperature_unit();
  thermo_entity->add_subscriber(this);
}

ThermoCard::ThermoCard(const std::string &uuid,
    const std::shared_ptr<Entity> &thermo_entity,
    const std::string &title) :
    Card(page_type::cardThermo, uuid, title),
    thermo_entity_(thermo_entity) {
  this->configure_temperature_unit();
  thermo_entity->add_subscriber(this);
}

ThermoCard::ThermoCard(
    const std::string &uuid,
    const std::shared_ptr<Entity> &thermo_entity,
    const std::string &title, const uint16_t sleep_timeout) :
    Card(page_type::cardThermo, uuid, title, sleep_timeout),
    thermo_entity_(thermo_entity) {
  this->configure_temperature_unit();
  thermo_entity->add_subscriber(this);
}

ThermoCard::~ThermoCard() {
  thermo_entity_->remove_subscriber(this);
}

void ThermoCard::accept(PageVisitor& visitor) { visitor.visit(*this); }

void ThermoCard::configure_temperature_unit() {
  if (Configuration::get_temperature_unit() == temperature_unit_t::celcius) {
    this->temperature_unit_icon_ = u8"\uE503"; // temperature-celsius
  } else {
    this->temperature_unit_icon_ = u8"\uE504"; // temperature-fahrenheit
  }
}

std::string &ThermoCard::render(std::string &buffer) {
  buffer.assign(this->get_render_instruction())
      .append(1, SEPARATOR)
      .append(this->get_title())
      .append(1, SEPARATOR);
  
  this->render_nav(buffer).append(1, SEPARATOR);

  buffer.append(this->thermo_entity_->get_entity_id());
  buffer.append(1, SEPARATOR);

  buffer.append(this->thermo_entity_->get_attribute(
    ha_attr_type::current_temperature));
  buffer.append(1, ' ');
  
  buffer.append(Configuration::get_temperature_unit_str());
  buffer.append(1, SEPARATOR);

  std::string dest_temp_str = 
    this->thermo_entity_->get_attribute(ha_attr_type::temperature);
  std::string dest_temp2_str;

  if (dest_temp_str.empty()) {
    dest_temp_str = this->thermo_entity_->get_attribute(
      ha_attr_type::target_temp_high, "0");
    dest_temp2_str = this->thermo_entity_->get_attribute(
      ha_attr_type::target_temp_low);
    if (!dest_temp2_str.empty()) {
      dest_temp2_str = std::to_string(
        static_cast<int>(std::stof(dest_temp2_str) * 10));
    }
  }
  dest_temp_str = std::to_string(
    static_cast<int>(std::stof(dest_temp_str) * 10));

  buffer.append(dest_temp_str).append(1, SEPARATOR);

  auto hvac_action = this->thermo_entity_->get_attribute(
    ha_attr_type::hvac_action);
  if (!hvac_action.empty()) {
    buffer
      .append(get_translation(hvac_action.c_str()))
      .append("\r\n(");
  }
  buffer.append(get_translation(this->thermo_entity_->get_state().c_str()));
  if (!hvac_action.empty()) {
    buffer.append(1, ')');
  }
  buffer.append(1, SEPARATOR);

  buffer.append(std::to_string(static_cast<int>(
    std::stof(this->thermo_entity_->get_attribute(
      ha_attr_type::min_temp, "0")) * 10)));
  buffer.append(1, SEPARATOR);

  buffer.append(std::to_string(static_cast<int>(
    std::stof(this->thermo_entity_->get_attribute(
      ha_attr_type::max_temp, "0")) * 10)));
  buffer.append(1, SEPARATOR);

  buffer.append(std::to_string(static_cast<int>(
    std::stof(this->thermo_entity_->get_attribute(
      ha_attr_type::target_temp_step, "0.5")) * 10)));
  
  //TODO: add overwrite_supported_modes
  auto& hvac_modes_str = 
    this->thermo_entity_->get_attribute(ha_attr_type::hvac_modes);
  if (hvac_modes_str.empty()) {
    buffer.append(4 * 8, SEPARATOR);
  } else {
    std::vector<std::string> hvac_modes;
    hvac_modes.reserve(6);
    split_str(',', hvac_modes_str, hvac_modes);

    for (auto& mode : hvac_modes) {
      uint16_t active_colour = 64512U;
      if (mode == ha_attr_hvac_mode::auto_ ||
          mode == ha_attr_hvac_mode::heat_cool) {
        active_colour = 1024U;
      } else if (mode == ha_attr_hvac_mode::off ||
          mode == ha_attr_hvac_mode::fan_only) {
        active_colour = 35921U;
      } else if (mode == ha_attr_hvac_mode::cool) {
        active_colour = 11487U;
      } else if (mode == ha_attr_hvac_mode::dry) {
        active_colour = 60897U;
      }
      auto icon = get_icon_by_name(CLIMATE_ICON_MAP, mode);
      buffer.append(1, SEPARATOR);
      buffer.append(icon == nullptr ? u8"\uE5D5" : icon).append(1, SEPARATOR);
      buffer.append(std::to_string(active_colour)).append(1, SEPARATOR);
      buffer.append(1, this->thermo_entity_->is_state(mode) ? '1' : '0');
      buffer.append(1, SEPARATOR);
      buffer.append(mode);
    }
    
    // todo: disperse icons evenly based on size of hvac_modes
    buffer.append(4 * (8 - hvac_modes.size()), SEPARATOR);
  }

  buffer.append(1, SEPARATOR);

  buffer.append(get_translation("currently")).append(1, SEPARATOR);
  buffer.append(get_translation("state")).append(1, SEPARATOR);
  // buffer.append(get_translation("action")).append(1, SEPARATOR); // depreciated
  buffer.append(1, SEPARATOR);
  buffer.append(this->temperature_unit_icon_).append(1, SEPARATOR);
  buffer.append(dest_temp2_str).append(1, SEPARATOR);
  
  if (this->thermo_entity_->has_attribute(ha_attr_type::preset_modes) || 
      this->thermo_entity_->has_attribute(ha_attr_type::swing_modes) || 
      this->thermo_entity_->has_attribute(ha_attr_type::fan_modes)) {
    buffer.append(1, '0');
  } else {
    buffer.append(1, '1');
  }

  return buffer;
}

} // namespace nspanel_lovelace
} // namespace esphome
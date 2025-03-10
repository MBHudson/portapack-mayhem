/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2023 Bernd Herzog
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "ui_flash_utility.hpp"
#include "portapack_shared_memory.hpp"

namespace ui {

Thread* FlashUtilityView::thread { nullptr };
static constexpr size_t max_filename_length = 26;

FlashUtilityView::FlashUtilityView(NavigationView& nav) : nav_ (nav) {
	add_children({
		&labels,
		&menu_view
	});

	menu_view.set_parent_rect({ 0, 3 * 8, 240, 33 * 8 });

	for (const auto& entry : std::filesystem::directory_iterator(u"FIRMWARE", u"*.bin")) {
		auto filename = entry.path().filename();
		auto path = entry.path().native();

		menu_view.add_item({
			filename.string().substr(0, max_filename_length),
			ui::Color::red(),
			&bitmap_icon_temperature,
			[this, path](KeyEvent) {
				this->firmware_selected(path);
			}
		});
	}
}

void FlashUtilityView::firmware_selected(std::filesystem::path::string_type path) {
	nav_.push<ModalMessageView>(
		"Warning!",
		"This will replace your\ncurrent firmware.\n\nIf things go wrong you are\nrequired to flash manually\nwith dfu.",
		YESNO,
		[this, path](bool choice) {
			if (choice) {
				std::u16string full_path = std::u16string(u"FIRMWARE/") + path;
				this->flash_firmware(full_path);
			}
		}
	);
}

void FlashUtilityView::flash_firmware(std::filesystem::path::string_type path) {
	ui::Painter painter;
		painter.fill_rectangle(
		{ 0, 0, portapack::display.width(), portapack::display.height() },
		ui::Color::black()
	);

	painter.draw_string({ 12, 24 }, this->nav_.style(), "This will take 15 seconds.");
	painter.draw_string({ 12, 64 }, this->nav_.style(), "Please wait while LEDs RX");
	painter.draw_string({ 12, 84 }, this->nav_.style(), "and TX are flashing.");
	painter.draw_string({ 12, 124 }, this->nav_.style(), "Then restart the device.");

	std::memcpy(&shared_memory.bb_data.data[0], path.c_str(), (path.length() + 1) * 2);
	m4_init(portapack::spi_flash::image_tag_flash_utility, portapack::memory::map::m4_code, false);
	m0_halt();
}

void FlashUtilityView::focus() {
	menu_view.focus();
}

} /* namespace ui */

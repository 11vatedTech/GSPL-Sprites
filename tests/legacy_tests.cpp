#include "gspl/legacy.hpp"
#include "gspl/source.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace { void check(bool value, const char* msg) { if (!value) throw std::runtime_error(msg); } }

int main() {
    try {
        // ---- 1. Detect legacy .sprite format ----
        {
            check(gspl::is_legacy_sprite_format("sprite MySprite\n"),
                  "Should detect 'sprite' prefix");
            check(gspl::is_legacy_sprite_format("SPRITE MySprite\n"),
                  "Should detect 'SPRITE' prefix");
            check(gspl::is_legacy_sprite_format("entity MyEntity\n"),
                  "Should detect 'entity' prefix");
            check(gspl::is_legacy_sprite_format("seed MySprite\n"),
                  "Should detect 'seed' prefix");
            check(!gspl::is_legacy_sprite_format("module test;\n"),
                  "Should not detect GSPL module as legacy");
            check(!gspl::is_legacy_sprite_format(""),
                  "Empty string should not be legacy");
        }

        // ---- 2. Parse legacy sprite content ----
        {
            auto buf = gspl::SourceBuffer::from_string("legacy.sprite",
                "sprite MySprite\n"
                "stable_id: \"com.example.mysprite\"\n"
                "name: \"My Sprite\"\n"
                "classification: \"test.sprite\"\n"
                "color: \"#FF0000\"\n"
                "rights: ORIGINAL_USER_CREATION\n"
                "form: default\n"
                "ability: shoot\n");
            gspl::LegacySpriteParser parser(buf);
            auto result = parser.parse();
            check(result.ok(), "Legacy parse should succeed");

            auto canonical = parser.to_canonical();
            check(canonical.stable_id == "com.example.mysprite", "stable_id should be parsed");
            check(canonical.name == "My Sprite", "name should be parsed");
            check(canonical.classification == "test.sprite", "classification should be parsed");
            check(canonical.primary_color == "#FF0000", "primary_color should be parsed");
            check(canonical.forms.size() == 1, "Should have 1 form");
            check(canonical.forms[0].id == "default", "Form should be 'default'");
            check(canonical.abilities.size() == 1, "Should have 1 ability");
            check(canonical.abilities[0].id == "shoot", "Ability should be 'shoot'");
        }

        // ---- 3. Migrate file (dry-run) ----
        {
            auto tmp = std::filesystem::temp_directory_path() / "gspl_legacy_test_3";
            std::filesystem::create_directories(tmp);
            auto input = tmp / "test.sprite";
            {
                std::ofstream f(input);
                f << "sprite TestEntity\n";
                f << "stable_id: \"test.migrate\"\n";
                f << "classification: \"test.migration\"\n";
            }

            gspl::MigrateOptions opts;
            opts.dry_run = true;

            auto report = gspl::migrate_file(input, opts);
            check(report.files_processed == 1, "Should process 1 file");
            check(report.success(), "Migration should succeed");

            std::filesystem::remove_all(tmp);
        }

        // ---- 4. Migrate file with output ----
        {
            auto tmp = std::filesystem::temp_directory_path() / "gspl_legacy_test_4";
            std::filesystem::create_directories(tmp);
            auto input = tmp / "mysprite.sprite";
            auto output_dir = tmp / "output";
            {
                std::ofstream f(input);
                f << "sprite Mysprite\n";
                f << "stable_id: \"test.output\"\n";
                f << "classification: \"test.output\"\n";
                f << "form: base\n";
            }

            gspl::MigrateOptions opts;
            opts.output_dir = output_dir;
            opts.overwrite = true;
            opts.verify_output = false;

            auto report = gspl::migrate_file(input, opts);
            check(report.files_converted == 1, "Should convert 1 file");
            check(report.success(), "Migration should succeed");
            check(std::filesystem::exists(output_dir / "mysprite.gspl"), "Output file should exist");

            std::filesystem::remove_all(tmp);
        }

        // ---- 5. Migrate non-existent file ----
        {
            gspl::MigrateOptions opts;
            auto report = gspl::migrate_file("nonexistent.sprite", opts);
            check(!report.success(), "Non-existent file should fail");
            check(report.errors.size() >= 1, "Should have at least 1 error");
        }

        // ---- 6. Migrate directory ----
        {
            auto tmp = std::filesystem::temp_directory_path() / "gspl_legacy_test_6";
            std::filesystem::create_directories(tmp);
            {
                std::ofstream f(tmp / "a.sprite");
                f << "sprite A\n";
            }
            {
                std::ofstream f(tmp / "b.sprite");
                f << "sprite B\n";
            }

            gspl::MigrateOptions opts;
            opts.output_dir = tmp / "out";
            opts.overwrite = true;
            opts.verify_output = false;

            auto report = gspl::migrate_directory(tmp, opts);
            check(report.files_processed == 2, "Should process 2 files");
            check(report.success(), "Directory migration should succeed");

            std::filesystem::remove_all(tmp);
        }

        // ---- 7. Non-legacy file should be skipped ----
        {
            auto tmp = std::filesystem::temp_directory_path() / "gspl_legacy_test_7";
            std::filesystem::create_directories(tmp);
            auto input = tmp / "modern.gspl";
            {
                std::ofstream f(input);
                f << "module modern;\nentity E { rights PUBLIC; morphology {} }\n";
            }

            gspl::MigrateOptions opts;
            opts.dry_run = true;

            auto report = gspl::migrate_file(input, opts);
            check(report.files_skipped >= 1, "Non-legacy file should be skipped");

            std::filesystem::remove_all(tmp);
        }

        std::cout << "ALL LEGACY COMPAT TESTS PASSED\n";
        return 0;
    } catch (std::exception const& e) {
        std::cerr << "FAILED: " << e.what() << '\n';
        return 1;
    }
}

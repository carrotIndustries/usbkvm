#include "about_dialog.hpp"
#include "version.hpp"


AboutDialog::AboutDialog() : Gtk::AboutDialog()
{
    std::string version = Version::get_string();
    if (strlen(Version::commit)) {
        version += "\nCommit " + std::string(Version::commit);
    }
    set_version(version);
    set_program_name("USBKVM");
    std::vector<Glib::ustring> authors;
    authors.push_back("Lukas K. <usbkvm@carrotIndustries.net>");
    set_authors(authors);
    set_license_type(Gtk::LICENSE_GPL_3_0);

    set_logo_icon_name("usbkvm");
}

from conan import ConanFile
from conan.tools.files import copy, get
import os


class GStreamerCppHelpersConan(ConanFile):
    name = "gstreamercpphelpers"
    description = "Headers to simplify GStreamer usage from C++."
    homepage = "https://github.com/nachogarglez/GStreamerCppHelpers"
    url = "https://github.com/nachogarglez/GStreamerCppHelpers"
    license = "LGPL-3.0"
    topics = ("gstreamer", "c++")
    package_type = "header-library"
    settings = "os", "arch", "compiler", "build_type"
    no_copy_source = True

    @property
    def _source_subfolder(self):
        return os.path.join(self.source_folder, "source_subfolder")

    def validate(self):
        if self.settings.get_safe("compiler.cppstd"):
            self.cpp_info.required_cpp_standard = 11

    def source(self):
        get(self, **self.conan_data["sources"][self.version], destination=self._source_subfolder, strip_root=True)

    def package(self):
        copy(self, "LICENSE", dst=os.path.join(self.package_folder, "licenses"), src=self._source_subfolder)
        copy(self, "*.h", dst=os.path.join(self.package_folder, "include"), src=self._source_subfolder)

    def package_id(self):
        self.info.clear()

    def package_info(self):
        self.cpp_info.includedirs = ["include"]

myDir = File.dirname(File.expand_path(__FILE__));
require "#{myDir}/../build-options.rb";

module Rakish

cfg = BuildConfig("root");

depends=[
    "#{cfg.thirdPartyPath}/oss-jextract",
    "#{cfg.artdlibPath}/artd-lib-logger",
    "#{cfg.artdlibPath}/artd-jlib-base",
    "#{cfg.artdlibPath}/artd-jlib-thread",
    "#{cfg.artdlibPath}/artd-lib-vecmath",
]

Rakish.Project(
	:includes => [
		Rakish::CppProjectModule, Rakish::GitModule
	],
	:name 		=> "artd-gpu-engine",
	:package 	=> "artd",
	:dependsUpon  => depends
) do

	cppDefine('BUILDING_webgpu_test');

    ifiles = addPublicIncludes("include/artd/*.h");

    export task :genProject => [ ifiles ]

end

end # Rakish
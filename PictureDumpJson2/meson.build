project('PictureDumpJson', 'cpp')

cpp = meson.get_compiler('cpp')
dep_libbe = cpp.find_library('be')
dep_rapidjson = dependency('RapidJSON')
dep_yamp_cpp = dependency('yaml-cpp')

lib_PictureDumpJson = static_library('PictureDumpJson',
	'PictureReaderBinary.cpp',
	'PictureWriterBinary.cpp',
	'PictureReaderJson.cpp',
	'PictureWriterJson.cpp',
	'PictureWriterYaml.cpp',
	'PictureReaderPlay.cpp',

	'JsonScanner.cpp',
	dependencies: [
		dep_libbe,
		dep_rapidjson,
		dep_yamp_cpp,
	],
	gnu_symbol_visibility: 'hidden',
)

dep_PictureDumpJson = declare_dependency(
	link_with: [lib_PictureDumpJson],
	dependencies: [
		dep_libbe,
		dep_rapidjson,
		dep_yamp_cpp,
	],
)

executable('PictureDumpJson',
	'PictureDump.cpp',
	dependencies: [
		dep_libbe,
		dep_PictureDumpJson,
	],
	gnu_symbol_visibility: 'hidden',
	install: true
)

executable('PictureDumpPlayJson',
	'PictureDumpPlay.cpp',
	dependencies: [
		dep_libbe,
		dep_PictureDumpJson,
	],
	gnu_symbol_visibility: 'hidden',
	install: true
)

executable('PictureCompileJson',
	'PictureCompile.cpp',
	dependencies: [
		dep_libbe,
		dep_PictureDumpJson,
	],
	gnu_symbol_visibility: 'hidden',
	install: true
)

executable('PictureCompileViewJson',
	'PictureCompile.cpp',
	dependencies: [
		dep_libbe,
		dep_PictureDumpJson,
	],
	gnu_symbol_visibility: 'hidden',
	install: true
)

executable('PictureJsonIdentity',
	'PictureJsonIdentity.cpp',
	dependencies: [
		dep_libbe,
		dep_PictureDumpJson,
	],
	gnu_symbol_visibility: 'hidden',
	install: true
)

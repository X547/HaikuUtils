project('PictureDumpJson', 'cpp')

cpp = meson.get_compiler('cpp')
dep_libbe = cpp.find_library('be')
dep_rapidjson = dependency('RapidJSON')
dep_yamp_cpp = dependency('yaml-cpp')

executable('PictureDumpJson',
	'PictureDump.cpp',
	'PictureReaderBinary.cpp',
	'PictureReaderJson.cpp',
	'PictureWriterBinary.cpp',
	'PictureWriterJson.cpp',
	'PictureWriterYaml.cpp',
	dependencies: [
		dep_libbe,
		dep_rapidjson,
		dep_yamp_cpp,
	],
	gnu_symbol_visibility: 'hidden',
	install: true
)

executable('PictureDumpPlayJson',
	'PictureDumpPlay.cpp',
	'PictureReaderPlay.cpp',
	'PictureWriterJson.cpp',
	dependencies: [
		dep_libbe,
		dep_rapidjson,
	],
	gnu_symbol_visibility: 'hidden',
	install: true
)

executable('PictureCompileJson',
	'PictureCompile.cpp',
	'PictureWriterBinary.cpp',
	'PictureReaderJson.cpp',
	dependencies: [
		dep_libbe,
		dep_rapidjson,
	],
	gnu_symbol_visibility: 'hidden',
	install: true
)

executable('PictureCompileViewJson',
	'PictureCompile.cpp',
	'PictureWriterBinary.cpp',
	'PictureWriterView.cpp',
	'PictureReaderJson.cpp',
	dependencies: [
		dep_libbe,
		dep_rapidjson,
	],
	gnu_symbol_visibility: 'hidden',
	install: true
)

executable('PictureJsonIdentity',
	'PictureJsonIdentity.cpp',
	'PictureWriterJson.cpp',
	'PictureReaderJson.cpp',
	dependencies: [
		dep_libbe,
		dep_rapidjson,
	],
	gnu_symbol_visibility: 'hidden',
	install: true
)

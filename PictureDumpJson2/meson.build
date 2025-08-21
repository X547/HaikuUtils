project('PictureDumpJson', 'cpp')

cpp = meson.get_compiler('cpp')
dep_libbe = cpp.find_library('be')
dep_rapidjson = dependency('RapidJSON')

executable('PictureDumpJson',
	'PictureDump.cpp',
	'PictureBinary.cpp',
	'PictureVisitorJson.cpp',
	dependencies: [
		dep_libbe,
		dep_rapidjson,
	],
	gnu_symbol_visibility: 'hidden',
	install: true
)

executable('PictureCompileJson',
	'PictureCompile.cpp',
	'PictureVisitorBinary.cpp',
	'PictureJson.cpp',
	dependencies: [
		dep_libbe,
		dep_rapidjson,
	],
	gnu_symbol_visibility: 'hidden',
	install: true
)

executable('PictureJsonIdentity',
	'PictureJsonIdentity.cpp',
	'PictureVisitorJson.cpp',
	'PictureJson.cpp',
	dependencies: [
		dep_libbe,
		dep_rapidjson,
	],
	gnu_symbol_visibility: 'hidden',
	install: true
)

project('EGLTest', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++14'])

cpp = meson.get_compiler('cpp')

libbe = cpp.find_library('be')
libOpenGL = dependency('opengl')
libEGL = dependency('egl')

executable(
	'EGLTest',
	'GlApp.cpp',
	'BitmapHookView.cpp',
	'MinApp.cpp',
	include_directories: [
		'/boot/system/develop/headers/private/shared',
	],
	dependencies: [
		libbe,
		libOpenGL,
		libEGL,
	],
	install: true
)

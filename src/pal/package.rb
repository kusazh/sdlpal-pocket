#!/usr/bin/env ruby
require 'json'
require 'digest'
require 'fileutils'
require 'optparse'
public_package = false
output = nil
OptionParser.new do |p|
  p.on('--public') { public_package = true }
  p.on('--output PATH') { |v| output = v }
end.parse!
abort 'Unexpected arguments' unless ARGV.empty?

root = File.expand_path('../..', __dir__)
out = output ? File.expand_path(output) : File.join(root, public_package ? 'build/public/pal/sdcard' : 'build/pocket/pal')
abort 'Public output must be new or empty' if public_package && Dir.exist?(out) && !Dir.children(out).empty?
core_id = 'Local.PAL'
platform = 'pal'
core = File.join(out, 'Cores', core_id)
assets = File.join(out, 'Assets', platform, 'common')
instances = File.join(out, 'Assets', platform, core_id)
platforms = File.join(out, 'Platforms')
template = File.join(root, 'src/sdk/platforms/pocket/templates')
manifest = File.readlines(File.join(root, 'runtime/MANIFEST')).map do |line|
  m = line.match(/^([0-9a-f]{32})\s+\.\/(.+)$/)
  [m[2], m[1]] if m
end.compact.to_h
%w[pocket/os25.rbf_r pocket/os.bin pocket/loader.bin].each do |name|
  next if public_package
  path = File.join(root, 'runtime', name)
  abort "Runtime manifest mismatch: #{name}" unless Digest::MD5.file(path).hexdigest == manifest.fetch(name)
end
abort 'Build the ELF first' unless File.file?(File.join(root, '.obj/pal/app.elf'))
abort 'Run make resources first' unless public_package || %w[pal.pak pal-fm.pcm].all? { |n| File.file?(File.join(__dir__, n)) }
[core, assets, instances, platforms].each { |d| FileUtils.mkdir_p(d) }
values = {'PLATFORM'=>platform, 'SHORT'=>'PAL', 'NAME'=>'SDLPAL', 'AUTHOR'=>'Local',
          'VERSION'=>'0.2.0', 'DATE'=>'2026-11-05', 'YEAR'=>'1995', 'BITSTREAM'=>'os25'}
%w[core audio video variants interact data input].each do |name|
  text = File.read(File.join(template, name + '.json'))
  values.each { |key, value| text = text.gsub('{{' + key + '}}', value) }
  doc = JSON.parse(text)
  if name == 'data'
    slot = doc.fetch('data').fetch('data_slots').find { |s| s['id'] == 5 }
    slot['name'] = 'DOS FM Music'
    slot['extensions'] = ['pcm']
  end
  if name == 'video'
    doc.fetch('video').fetch('scaler_modes').sort_by! { |m| m['width'] == 320 && m['height'] == 200 ? 0 : 1 }
  end
  if name == 'input'
    labels = {'pad_btn_a'=>'Confirm / Talk', 'pad_btn_b'=>'Cancel / Menu',
              'pad_btn_x'=>'Cancel / Menu', 'pad_btn_y'=>'Status',
              'pad_trig_l'=>'Previous Page', 'pad_trig_r'=>'Next Page',
              'pad_btn_start'=>'Cancel / Menu', 'pad_btn_select'=>'Auto Battle'}
    doc.fetch('input').fetch('controllers').each do |controller|
      controller.fetch('mappings').each { |m| m['name'] = labels.fetch(m['key'], m['name']) }
    end
  end
  File.write(File.join(core, name + '.json'), JSON.pretty_generate(doc) + "\n")
end
File.write(File.join(platforms, 'pal.json'), JSON.pretty_generate({platform: {
  category: 'Computer', name: 'SDLPAL', year: 1995, manufacturer: 'Softstar / SDLPAL'
}}) + "\n")
slots = [ [1,'os.bin'], [2,'pal.ini'], [3,'pal.elf'], [4,'pal.pak'], [5,'pal-fm.pcm'] ]
5.times { |i| slots << [10+i, "pal-#{i+1}.sav"] }
File.write(File.join(instances, 'PAL.json'), JSON.pretty_generate({instance: {
  magic: 'APF_VER_1', data_slots: slots.map { |id, filename| {id: id, filename: filename} }
}}) + "\n")
File.write(File.join(assets, 'pal.ini'), "[os]\nELF=pal.elf\nARGS=\nVARIANT=os25\n")
%w[os25.rbf_r loader.bin].each { |name| FileUtils.cp(File.join(root, 'runtime/pocket', name), core) } unless public_package
FileUtils.cp(File.join(root, 'runtime/pocket/os.bin'), assets) unless public_package
FileUtils.rm_f(File.join(assets, 'bank.ofsf'))
FileUtils.cp(File.join(__dir__, 'pal-fm.pcm'), assets) unless public_package
FileUtils.cp(File.join(root, '.obj/pal/app.elf'), File.join(assets, 'pal.elf'))
FileUtils.cp(File.join(__dir__, 'pal.pak'), assets) unless public_package
# Do not ship empty .sav files: copying an update must not replace progress.
puts "Ready: #{out} (#{public_package ? 'program only; runtime and game resources excluded' : 'local-use resources included'}; saves are not overwritten)"

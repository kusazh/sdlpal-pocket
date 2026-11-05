#!/usr/bin/env ruby
# Builds a local-use pack without modifying the original game directory.
require 'digest'
require 'json'
source, output = ARGV
abort 'usage: ruby pack.rb PAL_DIRECTORY pal.pak' unless source && output
names = %w[abc.mkf ball.mkf data.mkf f.mkf fbp.mkf fire.mkf gop.mkf map.mkf mgo.mkf midi.mkf mus.mkf pat.mkf rgm.mkf rng.mkf sss.mkf voc.mkf m.msg word.dat wor16.asc wor16.fon]
files = Dir.children(source).to_h { |n| [n.downcase, File.join(source, n)] }
entries = names.map { |name| [name, File.binread(files.fetch(name))] }
entries << ['sdlpal.cfg', File.binread(File.join(__dir__, 'sdlpal.cfg'))]
offset = 12 + entries.size * 32
table = entries.map do |name, data|
  entry = name.ljust(24, "\0") + [offset, data.bytesize].pack('V2')
  offset += data.bytesize
  entry
end.join
File.binwrite(output, "PALPAK1\0" + [entries.size].pack('V') + table + entries.map(&:last).join)
manifest = entries.map { |name, data| {name: name, size: data.bytesize, sha256: Digest::SHA256.hexdigest(data)} }
File.write(output + '.json', JSON.pretty_generate(manifest) + "\n")
puts "Packed #{entries.size} files, #{File.size(output)} bytes into #{output}"

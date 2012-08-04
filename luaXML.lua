x = os.clock()
result = parseXML("testdata.xml")
print(string.format("elapsed time: %.2f\n", os.clock() - x))

if result == nil then
  print("Error parseing XML")
else
  --printResult(result)
  print("Name:", result.name)
  print("Attr:", result.attr)
  print("Tags:", result.tags)
  print("Data:", result.data)

  print(result.tags[1].tags[1].tags[1].data)
end

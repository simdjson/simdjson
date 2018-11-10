
var fs = require('fs');

var faker = require('faker');


// generate bigDataSet as example
var bigSet = [];
var mmax = 500000
console.log("this may take some time...")
for(var i = 10; i < mmax; i++){
  if(i % 1024 == 0) process.stdout.write("\r"+i+" entries ("+Math.round(i * 100.0 /mmax)+" percent)");
  bigSet.push(faker.helpers.userCard());
};
console.log()

fs.writeFile(__dirname + '/large.json',  JSON.stringify(bigSet), function() {
  console.log("large.json generated successfully!");
})

function o = show_seq(num)

stest = sprintf("tests/test%02d.txt",num);
otest = sprintf("output/result_test%02d.txt",num);

s = load(stest);
[h w] = size(s);
if h>1 
  s = s';
endif;
o = load(otest);

#o = o(1:(200-length(s)-length(o)));

del = (max([s o])-min([s o]))*0.1;

axis([1 length(s)+length(o) min([s o])-del max([s o])+del]);
plot([s o],'r');
hold on;
plot(s,'b*-');
hold off;
axis('off');


print(sprintf("/tmp/report_test%02d.eps",num),"-color");


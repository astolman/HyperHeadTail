#Copyright (2016) Sandia Corporation
#Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#the U.S. Government retains certain rights in this software.
import old_utility
import rhcalculator
import sys
import pickle

for ccdh in sys.argv[1:]:
	df = old_utility.clean(ccdh)
	for k, item in df.iterrows():
		if item['category'] == 'Estimate':
			item['xs'].reverse()
			item['ys'].reverse()
			if item['xs'][-1] > 99999:
				print (ccdh + " is broken")
				sys.exit()
	for i, row in df.iterrows():
		if row['category'] != 'Estimate':
			continue
		print(row)
		e = rhcalculator.vectorize(list(zip(row['xs'], row['ys'])))
		target = df[(df['window'] == row['window']) & (df['category'] == 'True distro')].iloc[0]
		t = rhcalculator.vectorize(list(zip(target['xs'], target['ys'])))
		df.set_value(i, 'distance', rhcalculator.continuous_RH(e, t))
#	es = [rhcalculator.vectorize(list(zip(e['xs'], e['ys']))) for j, e in df.iterrows() if e['category'] == 'Estimate']
#	ts = [rhcalculator.vectorize(list(zip(e['xs'], e['ys']))) for _, e in df.iterrows() if e['category'] == 'True distro']
#	if len(es) > 1 or len(ts) > 1:
#		print(ccdh)
#	d = list(map(lambda e, t: rhcalculator.smart_find(e, t), es, ts))
#	temp = df[df['category'] == 'Estimate']
#	other = df[df['category'] == 'True distro']
	#temp['distance'] = d
	#df = temp.append(other)
	out = open(ccdh + '.pkl', 'wb')
	pickle.dump(df, out)
	out.close()

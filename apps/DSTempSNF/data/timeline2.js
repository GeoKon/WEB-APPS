		// State variables for this code
		
		var st = 			// DEFAULT STATE VALUES
		{
			dpos:   0,		// index to display data
			count: 10,		// number of datapoints
			lbinc:  1,
			xval:	0,		// value of axis at origin
		}
		// Function to call to initialize everything
		
		function initXAxis( v )				// v is in object passed by reference
		{
			st.count = v.npoints;
			st.lbinc = v.incr;
			st.xval  = v.first;
			
			for( var i=0; i<st.count; i++ )
			{
				myData.labels[i] = st.xval.toString();
				st.xval += st.lbinc;
			}	
		}
		function fillData( value )			// call every new datum
		{
			if( st.dpos<st.count )			// filling data for the first time
			{
				myData.datasets[0].data[ st.dpos++ ] = value;
			}
			else
			{
				for( var i=0; i<st.count-1; i++ )
				{
					// shift data
					myData.datasets[0].data[i] = myData.datasets[0].data[i+1];
					// shift axis
					myData.labels[i] = myData.labels[i+1];
				}
				st.dpos = st.count-1;
				myData.datasets[0].data[ st.dpos ] = value;
				
				myData.labels[ st.dpos ] = st.xval.toString();
				st.xval += st.lbinc;
				st.dpos = st.count;	// force to go to 'else' next time
			}
		}
		// --------------------- CHART PARAMETERS --------------------------
		var chartOptions = 
		{
			title: { display:true, fontSize:18, text:"Temperature Chart" },
			legend: { display: false, position: 'top' },
			animation: { duration: 0 },
			responsive: true,
			scales: 
			{
				yAxes: [{	//ticks: { min: -10, max: 80 } 
							ticks: { suggestedMin: 0, suggestedMax: 100 },
							scaleLabel: {display:true, labelString:"Temperature"}
						}],
				
				xAxes: [{ scaleLabel: {display:true, labelString:"Time"}}]
			}
		};
		var myData = 
		{
			labels:   [],	// X-AXIS LABELS
			datasets: [{				
				data: [],
				lineTension: 0.3,
				fill: false,
				borderColor: 'orange',
					  }]
		};
		var defaults = 
		{
			type: 'line', 
			data: myData, 				// see above
			options: chartOptions 		// see above
		}
		

		

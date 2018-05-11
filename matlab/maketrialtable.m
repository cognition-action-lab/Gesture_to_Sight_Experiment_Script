%make trial tables for blinded NAB

NTablePairs = 10;  %generate this many PAIRS of tables
NPracticeTables = NTablePairs;  %generate this many pairs of practice tables
Items = { [1:20]; [21:41] };  %each cell is a subgrouping of items (i.e., high vs low conflict)
PracticeItems = [41:45];
ItemDuration = 3000; %time that the item is displayed, in msec

rng('shuffle'); %reset the random number generator

matlabdir = cd;
cd ..
cd './TrialTables/'
curdir = cd;

%make the test trial tables
for a = 1:NTablePairs
    
    mkdir(curdir,sprintf('pair%d',a));
    newdir = sprintf('./pair%d/',a);
    
    %randomly shuffle the items in the list
    i1list = randperm(length(Items{1}));
    i2list = randperm(length(Items{2}));

    %get the corresponding items for the lists
    list1 = Items{1}(i1list);
    list2 = Items{2}(i2list);
    
    %group the first half of the two lists and the second half of the two lists
    listA = [list1(1:ceil(length(list1)/2)) list2(1:floor(length(list2)/2))]; 
    listB = [list1(ceil(length(list1)/2)+1:end) list2(floor(length(list2)/2)+1:end)]; 

    %do the same for each pair of subjects:
    for b = 1:2
        
        %we will generate 2 versions, one with vision and one without
        %vision
        for c = 0:1
        
            if c == 0
                visstr = 'NVF';
            elseif c == 1
                visstr = 'VF';
            end
            
            %randomize the lists
            listA = listA(randperm(length(listA)));
            listB = listB(randperm(length(listB)));
            
            %print out the lists. if c == 0, no vision; if c == 1, vision
            fid = fopen([newdir sprintf('tbl%d-%d_list%s_%s.txt',a,b,'A',visstr)],'wt');
            for d = 1:length(listA)
                fprintf(fid,'%d %d %d %d\n',c,listA(d),ItemDuration,1); %note, for "practice" flag, logic is inverted
            end
            fclose(fid);
            
            fid = fopen([newdir sprintf('tbl%d-%d_list%s_%s.txt',a,b,'B',visstr)],'wt');
            for d = 1:length(listB)
                fprintf(fid,'%d %d %d %d\n',c,listB(d),ItemDuration,1); %note, for "practice" flag, logic is inverted
            end
            fclose(fid);
        end
    end
end


        
%make the practice trial tables
for a = 1:NPracticeTables
    
    newdir = sprintf('./pair%d/',a);
    
    %randomly shuffle the items in the list
    listA = randperm(length(PracticeItems));
    listB = randperm(length(PracticeItems));
    
    %randomize the lists
    %listA = listA(randperm(length(listA)));
    %listB = listB(randperm(length(listA)));
    %get the items for the randomized lists
    listA = PracticeItems(listA);
    listB = PracticeItems(listB);
    
    %print out the lists. if c == 0, no vision; if c == 1, vision
    fid = fopen([newdir sprintf('practice%d_%s.txt',a,'VF')],'wt');
    for d = 1:length(listA)
        fprintf(fid,'%d %d %d %d\n',1,listA(d),ItemDuration,0); %note, for "practice" flag, logic is inverted
    end
    fclose(fid);
    
    fid = fopen([newdir sprintf('practice%d_%s.txt',a,'NVF')],'wt');
    for d = 1:length(listB)
        fprintf(fid,'%d %d %d %d\n',0,listB(d),ItemDuration,0); %note, for "practice" flag, logic is inverted
    end
    fclose(fid);
end

cd(matlabdir);
        
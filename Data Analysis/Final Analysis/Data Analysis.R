# set work directory to where all of the raw data and survey results are
setwd("C:/Users/rache/Desktop/github/Wrist-Actigraphy-Sleep-Tracking-System/Data Analysis/Final Analysis")

# load survey data
data<-read.csv("survey_results.csv", fileEncoding = 'UTF-8-BOM')
colnames(data)

library(dplyr)
library(ggplot2)
library(lattice)

# read data, UTF8 to fix special characters
p1n1<-read.csv("processed_p1n1.csv", fileEncoding = 'UTF-8-BOM')
p1n2<-read.csv("processed_p1n2.csv", fileEncoding = 'UTF-8-BOM')
p1n3<-read.csv("processed_p1n3.csv", fileEncoding = 'UTF-8-BOM')
p2n1<-read.csv("processed_p2n1.csv", fileEncoding = 'UTF-8-BOM')
p2n2<-read.csv("processed_p2n2.csv", fileEncoding = 'UTF-8-BOM')
p2n3<-read.csv("processed_p2n3.csv", fileEncoding = 'UTF-8-BOM')
p3n1<-read.csv("processed_p3n1.csv", fileEncoding = 'UTF-8-BOM')
p3n2<-read.csv("processed_p3n2.csv", fileEncoding = 'UTF-8-BOM')
p5n1<-read.csv("processed_p5n1.csv", fileEncoding = 'UTF-8-BOM')
p5n2<-read.csv("processed_p5n2.csv", fileEncoding = 'UTF-8-BOM')
p5n3<-read.csv("processed_p5n3.csv", fileEncoding = 'UTF-8-BOM')
p6n1<-read.csv("processed_p6n1.csv", fileEncoding = 'UTF-8-BOM')
p7n1<-read.csv("processed_p7n1.csv", fileEncoding = 'UTF-8-BOM')
p7n2<-read.csv("processed_p7n2.csv", fileEncoding = 'UTF-8-BOM')
total<-rbind(p1n1,p1n2,p1n3,p2n1,p2n2,p2n3,p3n1,p3n2,p5n1,p5n2,p5n3,p6n1,p7n1,p7n2)
colnames(total)

#give each trial a unique persontrial number
total %>% mutate(persontrial=person*10+trial)->newtotal
data %>% mutate(persontrial=person*10+trial)->newdata
left_join(newtotal,newdata,by=c("persontrial"))->all

#graph then save to graphs folder
x <- c("11","12","13","21","22","23","31","32","51","52","53","61","71","72") # insert every persontrial number

dir.create("Graphs")
setwd("C:/Users/rache/Desktop/github/Wrist-Actigraphy-Sleep-Tracking-System/Data Analysis/Graphs")

for (val in x) {
  all.graph<-all %>% group_by(persontrial) %>% filter(persontrial==val) %>% mutate(noise.index=ifelse(Mean<quantile(Mean,.25)-1.5*IQR(Mean)|Mean>quantile(Mean,.75)+1.5*IQR(Mean),1,0))
  table(all.graph$noise.index)
  plot <- ggplot(data=all.graph,legend=NULL,aes(x=Epoch,y=Mean,color=noise.index))+geom_line()
  plot <- plot+labs(title="Average Act(t) by Epoch", caption=paste("Person ",substring(val,1,1),"Trial ",substring(val,2,2)))
  plot
  ggsave(paste("p",substring(val,1,1),"n",substring(val,2,2),".png"),sep = "")
}

#go back to main directory
setwd("C:/Users/rache/Desktop/github/Wrist-Actigraphy-Sleep-Tracking-System/Data Analysis")


#create noise.index from mean activity, classify epochs into sleep(0) or wake(1)
data.all<-all %>% group_by(persontrial) %>% mutate(noise.index=ifelse(Mean<quantile(Mean,.25)-1.5*IQR(Mean)|Mean>quantile(Mean,.75)+1.5*IQR(Mean),1,0))
data.all<-new.all %>% group_by(persontrial) %>% mutate(total.sleep=max(Epoch))

as.data.frame(table(new.all$persontrial,new.all$noise.index))->sleep.efficiency
data.all<-data.all %>% select(total.sleep,noise.index,quality,age,gender,timewake,timebefore,length,persontrial) 
left_join(data.all,sleep.efficiency,by=c("persontrial"))
write.csv(unique.data.frame(data.all),"combined_data.csv") # combined data for easy access later


find.sleep.moment <- function(noise.index.vector, n.times) {
  #find initial moment of sleep where there are n consecutive 0 (sleep) epochs
  a <- rle(noise.index.vector)
  rle.sleep <- as.data.frame(cbind("value" = a$values, "length" = a$lengths))
  n <- which(rle.sleep$value == 0 & rle.sleep$length >= n.times)
  sleep.moment <- sum(rle.sleep$length[1: n[1]-1]) + n.times
  return(sleep.moment)
}

# test the function
data<-read.csv("combined_data.csv", fileEncoding = 'UTF-8-BOM')
colnames(data)
head(data)
options(digits=2)
cor(data[,c(3,6,7,8,13,14,15)])

cor.test(data$length,data$count)
